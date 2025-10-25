#include "main.hpp"
#include "config.hpp"
#include "mpp.hpp"

#include <filesystem>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <memory>
#include <map>

namespace fs = std::filesystem;

namespace EuropaBuild
{
    BuildTool::BuildTool(std::shared_ptr<const BuildConfig2> config)
        : _config(config)
    {
		log.log("EuropaBuild C++\n", ESLogVerbosity::Warning, ESLog::CYAN);
		log.log("Dependency graph: \n" + _config->tree->getWholeDependecyTreeAsString(), ESLogVerbosity::Verbose, ESLog::BLUE);
	}

    std::unique_ptr<MPP::Compiler> detectCompiler()
    {
        const std::vector<std::string> compilers = { "g++", "clang++", "c++" };

        for (const auto& compiler_name : compilers)
        {
            auto compiler = MPP::detect_compiler(MPP::ELanguage::CPP, MPP::EMachine::BUILD, { compiler_name });
            if (compiler)
            {
                return compiler;
            }
        }

        return nullptr;
    }

    void writeCompilerRule(std::ofstream& out, MPP::Compiler* compiler)
    {
        out << "rule cpp_compile" << std::endl << "  command =";

        for (const auto& cmd : compiler->command)
        {
            out << " " << cmd;
        }

        // add compile-only and output commands
        auto compile_cmd = compiler->compile_only_command();
        for (const auto& arg : compile_cmd)
        {
            out << " " << arg;
        }

        out << " ${ARGS} -o ${out} ${in}" << std::endl
            << "  description = Compiling C++ object ${out}" << std::endl
            << std::endl;
    }

    void writeLinkerRule(std::ofstream& out, MPP::Compiler* compiler)
    {
        out << "rule cpp_link" << std::endl << "  command =";

        for (const auto& cmd : compiler->command)
        {
            out << " " << cmd;
        }

        out << " ${ARGS} -o ${out} ${in}" << std::endl
            << "  description = Linking executable ${out}" << std::endl
            << std::endl;
    }

    void writeArchiverRule(std::ofstream& out, MPP::Compiler* compiler)
    {
        out << "rule cpp_archive" << std::endl
            << "  command = ar rcs ${out} ${in}" << std::endl
            << "  description = Creating static library ${out}" << std::endl
            << std::endl;
    }

	fs::path escapeSpacesForNinja(const fs::path& p)
	{
		// escape spaces with the ninja escape character $
		std::string str = p.string();
		static const char space = 32;
		static const std::string spaceEsc = "$ ";
		size_t pos = str.find(space);
		while (pos != std::string::npos)
		{
			str.replace(pos, 1, spaceEsc);
			pos = str.find(space, pos + spaceEsc.length());
		}
		return fs::path(str);
	}

    std::string sourceFilePathToObjFilenameString(const fs::path& objOutDir, const fs::path& sourcePath)
    {
		fs::path outPath = escapeSpacesForNinja(sourcePath);
		outPath = fs::path(outPath.stem().string() + ".o"); // add .o file extension, remove directory
		return (objOutDir / outPath).string(); // the new path is where the generated object file goes
    }

    void generateNinjaBuild(const BuildConfig2& _config, std::shared_ptr<std::vector<TargetMapping>> mappings, MPP::Compiler* compiler)
    {
        std::ofstream out(fs::current_path() / "build.ninja");

        out << "# EuropaBuild C++ Generated build file" << std::endl
            << "# ====================================" << std::endl
            << std::endl
            << "ninja_required_version = 1.8.2" << std::endl
            << std::endl;

        writeCompilerRule(out, compiler);
        writeLinkerRule(out, compiler);
        writeArchiverRule(out, compiler);
        out << std::endl;

        std::map<std::string, std::vector<std::string>> depObjectFiles;
        for (const TargetMapping& mapping : *mappings)
        {
            const auto& target = *mapping.target;
            std::vector<std::string> object_files;
            for (const auto& source_file : mapping.sourceFiles)
            {
				std::string obj_file = sourceFilePathToObjFilenameString(fs::path(INTERMEDIATE_DIR), fs::path(source_file));
                object_files.push_back(obj_file);
				out << "build " << obj_file << ": cpp_compile " << escapeSpacesForNinja(fs::path(source_file)).string() << std::endl;
                out << "  ARGS =";
                out << std::endl << std::endl;
            }

            if (target.targetType == ETargetType::Dependency)
            {
                // all targets should be in order at this point, 
                // so if a later target depends on this one it will be able to find these object files to link against
                depObjectFiles[target.name] = object_files;
            }
            else if (target.targetType == ETargetType::Executable or target.targetType == ETargetType::StaticLib)
            {
                // executables and libraries (archives) need to actually be linked, not just compiled
                if (target.targetType == ETargetType::Executable)
                    out << "build " << target.name << ": cpp_link";
                else
                    out << "build " << target.name << ": cpp_archive";

                for (const auto& obj : object_files)
                    out << " " << obj;

                // also link against object files from dependency-targets built before this one
                // the other target that is the dependency must be marked as such in the dependencies list for this target
                if (target.depends.size() > 0)
                {
                    for (const auto& depName : target.depends)
                    {
                        if (depObjectFiles.find(depName) == depObjectFiles.end())
                            throw DependencyException("Target " + target.name + " is set to depend on " + depName + " but the latter could not be found");

                        for (const auto& obj : depObjectFiles[depName])
                            out << " " << obj;
                    }
                }
                out << std::endl << std::endl;
            }
            else if (target.targetType == ETargetType::DynamicLib)
            {
                throw std::runtime_error("DLLs not yet supported");
            }

        }

        // targets that no others depend on are "defaults" (final products)
        const auto& products = _config.tree->targetsThatAreFinalProducts;
        if (products.size() < 1)
            throw DependencyException("Could not determine any default target because all targets are used as dependencies");

        out << "default ";
        for (const std::string& d : products)
            out << d << " ";
        out << std::endl;
    }

    int BuildTool::build()
    {
        try
        {
            // pick a compiler
            auto compiler = detectCompiler();
            if (not compiler)
                throw EnvironmentException("No suitable C++ compiler found");
            log.log("Using compiler " + compiler->id() + "\n");

            log.log("Generating " + std::to_string(_config->tree->size()) + " targets");

            std::shared_ptr<std::vector<TargetMapping>> targetMappings = std::make_shared<std::vector<TargetMapping>>();
            
            size_t totalSourceFiles = 0;
            for (const auto& target : *(_config->tree))
            {
                targetMappings->push_back(TargetMapping());
                TargetMapping& mapping = targetMappings->back();
                mapping.target = target;
                // discover c++ source files
                mapping.sourceFiles = discoverSourceFiles(mapping.target->sources);
                if (mapping.sourceFiles.empty())
                    throw ConfigException("No C++ source files found for target " + target->name);
                totalSourceFiles += mapping.sourceFiles.size();

				createRelativeDirectory(target->outputPath);
            }

            log.log("Found " + std::to_string(totalSourceFiles) + " sources");

            // generate ninja build file
            generateNinjaBuild(*_config, targetMappings, compiler.get());

            createRelativeDirectory(fs::current_path() / _config->intermediateDir);

            log.log("Build configuration generated", ESLogVerbosity::Verbose, ESLog::CYAN);

            return runNinja();
        }
        catch (const std::exception & e)
        {
            log.error("Build error: " + std::string(e.what()));
            return 1;
        }
    }

    ESLogVerbosity BuildTool::getLogVerbosity() const
    {
        return log.verbosity;
    }
    
    std::vector<fs::path> BuildTool::discoverSourceFiles(const std::vector<fs::path>& paths)
    {
        std::vector<fs::path> source_files;

        for (const auto& subdir : paths)
        {
			const auto relpath = fs::current_path() / subdir;
            for (const auto& entry : fs::recursive_directory_iterator(relpath))
            {
                if (entry.is_regular_file()) 
                {
                    const auto& path = entry.path();
                    const auto ext = path.extension();
                    if (ext == ".cpp" || ext == ".cxx" || ext == ".cc" || ext == ".c++")
                    {
                        source_files.push_back(fs::relative(path, fs::current_path()));
                    }
                }
            }
        }

        std::sort(source_files.begin(), source_files.end());
        return source_files;
    }
    
    int8_t BuildTool::runNinja()
    {
        std::cout << "Running Ninja" << std::endl;
        auto const& [ret, out, err] = MPP::process(std::vector<std::string>{ "ninja" });
        if (ret != 0) 
        {
            std::cerr << ESLog::colorMessage("Command failed (code " + std::to_string(ret) + ")\n" + err + "\n", ESLog::BRIGHT_RED);
            return ret;
        }
        std::cout << out << std::endl;
        std::cout << "Build completed" << std::endl;
        return 0;
    }

    void BuildTool::createRelativeDirectory(const fs::path& path)
    {
        if (not fs::exists(fs::current_path() / path))
            fs::create_directories(fs::current_path() / path);
    }

} // namespace EuropaBuild

int main(int argc, char* argv[]) 
{
    try
    {
        fs::path configFilePath = fs::current_path() / "EuropaBuild.json";
        std::shared_ptr<EuropaBuild::BuildConfig2> config = std::make_shared<EuropaBuild::BuildConfig2>();
        config->intermediateDir = EuropaBuild::INTERMEDIATE_DIR;
        if (argc <= 1)
            config = EuropaBuild::ConfigUtils::parseConfigFromJson(configFilePath);
        else
            throw std::runtime_error("arguments not currently supported");

        EuropaBuild::BuildTool buildTool(config);
        return buildTool.build();
    }
    catch (const std::exception& e)
    {
        std::cerr << EuropaBuild::ESLog::colorMessage("Error: " + std::string(e.what()), EuropaBuild::ESLog::BRIGHT_RED);
        return 1;
    }

    std::cout << EuropaBuild::ESLog::colorMessage("All done!", EuropaBuild::ESLog::CYAN);
    return 0;
}

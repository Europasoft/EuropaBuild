#include "EuropaBuild/build.hpp"
#include "EuropaBuild/tree.hpp"
#include "EuropaBuild/findtool.hpp"
#include "EuropaBuild/main.hpp"
#include "util/process.hpp"

#include <cstdio>
#include <string>
#include <algorithm>
#include <map>

namespace EuropaBuild
{
	BuildTool::BuildTool(std::shared_ptr<const BuildTree> treePtr)
		: tree(treePtr)
	{
		log("----------------------------------\nDependency graph");
		log(tree->getWholeDependecyTreeAsString(), LogColors::BLUE);
		log("----------------------------------");
	}

	int BuildTool::build()
	{
		using namespace FindTool;
		try
		{
			// pick a compiler and archiver
			std::shared_ptr<Toolchain> toolchain = Toolchain::selectToolchain();
			if (not toolchain->compiler)
			{
				throw EnvironmentException("No suitable compiler found");
			}
			log("\nUsing compiler " + toolchain->compiler->name + "\n", LogColors::CYAN);

			log("Mapping " + std::to_string(tree->size()) + " targets");

			std::shared_ptr<std::vector<TargetMapping>> targetMappings = std::make_shared<std::vector<TargetMapping>>();

			size_t totalSourceFiles = 0;
			for (const auto& target : *(tree))
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

			log("Found " + std::to_string(totalSourceFiles) + " sources");

			// generate ninja build file
			generateNinjaBuild(*tree, targetMappings, toolchain);

			createRelativeDirectory(GET_INTERMEDIATE_PATH());

			log("Build rules generated");

			return runNinja();
		}
		catch (const std::exception& e)
		{
			log("Build error: " + std::string(e.what()), LogColors::BRIGHT_RED);
			return 1;
		}
	}

	std::vector<fs::path> BuildTool::discoverSourceFiles(const std::vector<fs::path>& paths)
	{
		std::vector<fs::path> source_files;

		for (const auto& subdir : paths)
		{
			const auto relpath = fs::current_path() / subdir;

			// each source path in the json file may be either a directory path or a path to a specific cpp file
			if (fs::is_directory(relpath))
			{
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
			else if (fs::is_regular_file(relpath))
			{
				const auto ext = relpath.extension();
				if (ext == ".cpp" || ext == ".cxx" || ext == ".cc" || ext == ".c++")
				{
					source_files.push_back(fs::relative(relpath, fs::current_path()));
				}
			}
		}

		std::sort(source_files.begin(), source_files.end());
		source_files.erase(std::unique(source_files.begin(), source_files.end()), source_files.end());
		return source_files;
	}

	void BuildTool::generateNinjaBuild(const BuildTree& tree, std::shared_ptr<std::vector<TargetMapping>> mappings, std::shared_ptr<FindTool::Toolchain> toolchain)
	{
		std::ofstream out(fs::current_path() / NINJA_FILENAME);

		out << "# EuropaBuild C++ Generated build file" << std::endl
			<< "# ====================================" << std::endl
			<< std::endl
			<< "ninja_required_version = 1.8.2" << std::endl
			<< std::endl;

		writeCompilerRule(out, toolchain);
		writeLinkerRule(out, toolchain);
		writeArchiverRule(out, toolchain);
		out << std::endl;

		size_t sourceFileCounter = 0;
		std::map<std::string, std::vector<std::string>> depObjectFiles;
		for (const TargetMapping& mapping : *mappings)
		{
			const Target& target = *mapping.target;
			std::vector<std::string> objectFiles;
			for (const fs::path& sourceFilePath : mapping.sourceFiles)
			{
				// source file compile command
				std::string objFile = sourceFilePathToObjFilenameString(fs::path(sourceFilePath), std::to_string(sourceFileCounter));
				sourceFileCounter++;
				objectFiles.push_back(objFile);
				out << "build " << objFile << ": cpp_compile " << escapeSpacesForNinja(fs::path(sourceFilePath)).string() << std::endl;
				out << "  ARGS =" << includePathArgs(mapping) << " -std=c++20";
				out << std::endl << std::endl;
			}

			const bool isExecutable = target.targetType == ETargetType::Executable;
			const bool isDependency = target.targetType == ETargetType::Dependency;
			const bool isStaticLib = target.targetType == ETargetType::StaticLib;
			const bool isDynamicLib = target.targetType == ETargetType::DynamicLib;

			if (isDependency or isStaticLib or isDynamicLib)
			{
				// all targets should be in order at this point, 
				// so if a later target depends on this one it will be able to find these object files to link against
				depObjectFiles[target.name] = objectFiles;
			}

			if (isExecutable or isStaticLib or isDynamicLib)
			{
				// executables and libraries (archives) need to actually be linked, not just compiled
				const std::string targetOutputPath = makeTargetFullOutputPath(target);
				if (isExecutable || isDynamicLib)
				{
					out << "build " << targetOutputPath << ": cpp_link";
				}
				else
				{
					out << "build " << targetOutputPath << ": cpp_archive";
				}

				// write the target's own compiled object files
				for (const std::string& obj : objectFiles)
				{
					out << " " << obj;
				}
				// also link against object files from dependency-targets built before this one
				// the other target that is the dependency must be marked as such in the dependencies list for this target
				std::vector<std::string> implicitDeps; // to track the built files we depend on
				for (const auto& depName : target.depends)
				{
					// HANDLING ANOTHER TARGET THAT IS LISTED AS A DEPENDENCY OF THIS TARGET
					if (depObjectFiles.find(depName) == depObjectFiles.end())
					{
						throw DependencyException("Target " + target.name + " is set to depend on " + depName + " but the latter could not be found");
					}
					// find the target dependency to see what type it is
					auto depTargetIt = std::find_if(mappings->begin(), mappings->end(),
						[&depName](const TargetMapping& m) { return m.target->name == depName; });

					if (depTargetIt != mappings->end())
					{
						const Target& depTarget = *depTargetIt->target;

						if (depTarget.targetType == ETargetType::Dependency)
						{
							// "dependency" targets get directly compiled into this target
							for (const auto& obj : depObjectFiles[depName])
							{
								out << " " << obj;
							}
						}
						else if (depTarget.targetType == ETargetType::StaticLib || depTarget.targetType == ETargetType::DynamicLib)
						{
							// Ninja needs to know that this is an "implicitly dependency",
							// since the library binary file must be fully compiled and linked before the target that depends on it
							implicitDeps.push_back(makeTargetFullOutputPath(depTarget));
						}
					}
				}

				// append Ninja implicit dependencies if there are any
				if (!implicitDeps.empty())
				{
					out << " |"; // pipes indicate implicit dependencies in Ninja
					for (const auto& depPath : implicitDeps)
					{
						out << " " << depPath;
					}
				}

				if (isExecutable || isDynamicLib)
				{
					out << std::endl;
					out << "  ARGS =" << libraryArgs(target);
				}

				out << std::endl << std::endl;
			}
			if (isDynamicLib)
			{
				throw std::runtime_error("DLLs not yet supported");
			}

		}

		// targets that no others depend on are "defaults" (final products)
		const Targets& products = tree.targetsThatAreFinalProducts;
		if (products.size() < 1)
			throw DependencyException("Could not determine any default target because all targets are used as dependencies");

		out << "default ";
		for (const std::shared_ptr<const Target>& p : products)
			out << makeTargetFullOutputPath(*p) << " ";
		out << std::endl;
	}

	void BuildTool::writeCompilerRule(std::ofstream& out, std::shared_ptr<FindTool::Toolchain> toolchain)
	{
		out << "rule cpp_compile" << std::endl << "  command =";
		out << " " << toolchain->compiler->command;

		out << " " << toolchain->compiler->compileFlag;

		out << " ${ARGS} -o ${out} ${in}" << std::endl
			<< "  description = Compiling C++ object ${out}" << std::endl
			<< std::endl;
	}

	void BuildTool::writeLinkerRule(std::ofstream& out, std::shared_ptr<FindTool::Toolchain> toolchain)
	{
		out << "rule cpp_link" << std::endl << "  command =";
		out << " " << toolchain->compiler->command;

		out << " ${ARGS} -o ${out} ${in}" << std::endl
			<< "  description = Linking executable ${out}" << std::endl
			<< std::endl;
	}

	void BuildTool::writeArchiverRule(std::ofstream& out, std::shared_ptr<FindTool::Toolchain> toolchain)
	{
		out << "rule cpp_archive" << std::endl
			<< "  command = ar rcs ${out} ${in}" << std::endl
			<< "  description = Creating static library ${out}" << std::endl
			<< std::endl;
	}

	std::string BuildTool::sourceFilePathToObjFilenameString(const fs::path& sourcePath, std::string suffix)
	{
		fs::path filename = escapeSpacesForNinja(sourcePath);
		filename = fs::path(filename.stem().string() + suffix + ".o"); // add .o file extension, remove directory
		return (INTERMEDIATE_DIR_NAME / filename).string(); // the new path is where the generated object file goes
	}

	std::string BuildTool::makeTargetFullOutputPath(const Target& target)
	{
		std::string ext;
		if (target.targetType == ETargetType::Executable)
			ext = WinOS ? ".exe" : ".bin";
		if (target.targetType == ETargetType::StaticLib)
			ext = WinOS ? ".lib" : ".a";
		if (target.targetType == ETargetType::DynamicLib)
			ext = WinOS ? ".dll" : ".so";

		return escapeSpacesForNinja(target.outputPath / fs::path(target.name + ext)).string();
	}

	int8_t BuildTool::runNinja()
	{
		log("\nRunning Ninja...");
		auto const& [ret, out, err] = Util::process(std::vector<std::string>{ "ninja" });
		if (ret != 0)
		{
			// handle ninja error
			std::string detailedError = "Ninja error (code " + std::to_string(ret) + ")\n\n";

			// info from stderr
			if (!err.empty())
			{
				detailedError += "--- Captured output (stderr) ---\n" + err + "\n";
			}
			// info from stdout
			if (!out.empty())
			{
				detailedError += "--- Captured output (stdout) ---\n" + out + "\n";
				// provide hints in some specific known error cases
				if (out.find("STL1000") != std::string::npos || out.find("Unexpected compiler version") != std::string::npos)
				{
					detailedError += "\nTip: Another compiler version is required.\nPlease update the compiler.\n";
				}
			}
			else if (err.empty())
			{
				detailedError += "No error information to show. Make sure Ninja is installed and that that 'ninja' command is usable.\n";
			}

			throw EnvironmentException(detailedError);
		}

		log(out, LogColors::BLUE);
		log("Build completed", LogColors::CYAN);
		return 0;
	}

	void BuildTool::createRelativeDirectory(const fs::path& path)
	{
		if (not fs::exists(fs::current_path() / path))
			fs::create_directories(fs::current_path() / path);
	}

	fs::path BuildTool::escapeSpacesForNinja(const fs::path& p)
	{
		// escape spaces with the ninja escape character $
		return fs::path(escapeSpacesForNinja(p.string()));
	}

	std::string BuildTool::escapeSpacesForNinja(std::string s)
	{
		// escape spaces with the ninja escape character $
		static const char space = 32;
		static const std::string esc = "$ ";
		size_t pos = s.find(space);
		while (pos != std::string::npos)
		{
			s.replace(pos, 1, esc);
			pos = s.find(space, pos + esc.length());
		}
		return s;
	}

	std::string BuildTool::includePathArgs(const TargetMapping& mapping)
	{
		std::string inArgs;
		for (const fs::path& in : mapping.target->includePaths)
		{
			inArgs += " \"-I" + in.string() + "\"";
		}
		return inArgs;
	}

	std::string BuildTool::libraryArgs(const Target& target)
	{
		std::string libArgs;
		// most linkers, including clang++ on windows, accept standard -L flags
		for (const fs::path& libPath : target.libPaths)
		{
			libArgs += " \"-L" + libPath.string() + "\"";
		}

		// link the individual libraries
		for (const std::string& libr : target.libs)
		{
			std::string lib = escapeSpacesForNinja(libr);
			// If it ends in .lib or .a, strip the extension and prepend -l
			if (lib.size() > 4 && (lib.compare(lib.size() - 4, 4, ".lib") == 0 || lib.compare(lib.size() - 2, 2, ".a") == 0))
			{
				std::string stem = lib.substr(0, lib.find_last_of('.'));
				libArgs += " -l" + stem;
			}
			else if (lib.find('.') != std::string::npos || lib.rfind("-l", 0) == 0)
			{
				libArgs += " " + lib;
			}
			else
			{
				libArgs += " -l" + lib;
			}
		}

		return libArgs;
	}

} // namespace EuropaBuild
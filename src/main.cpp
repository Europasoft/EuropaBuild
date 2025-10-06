
#include <filesystem>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>

#include "main.hpp"
#include "mpp.hpp"


namespace fs = std::filesystem;

namespace EuropaBuild
{
	EuropaBuild::EuropaBuild(const BuildConfig & config) : config_(config)
    {}

    int EuropaBuild::build()
    {
        try
        {
            std::cout << "EuropaBuild C++" << std::endl
                      << "Source dir: " << fs::absolute(config_.source_dir).string()
                      << std::endl
                      << "Build dir: " << fs::absolute(config_.build_dir).string()
                      << std::endl
                      << "Output: " << config_.output_name << std::endl
                      << "Type: " << config_.build_type << std::endl;

            // create build directory
            if (!fs::exists(config_.build_dir))
            {
                fs::create_directories(config_.build_dir);
            }

            // discover c++ source files
            auto source_files = discoverSourceFiles();
            if (source_files.empty())
            {
                std::cerr << "No C++ source files found in " << config_.source_dir << std::endl;
                return 1;
            }

            std::cout << "Found " << source_files.size() << " source files" << std::endl;

            // detect compiler
            auto compiler = detectCompiler();
            if (!compiler)
            {
                std::cerr << "No suitable C++ compiler found" << std::endl;
                return 1;
            }

            std::cout << "Using compiler: " << compiler->id() << std::endl;

            // generate ninja build file
            generateNinjaBuild(source_files, compiler.get());

            std::cout << "Build configuration generated" << std::endl;
            std::cout << "Run 'ninja' in the build directory to compile" << std::endl;

            return 0;
        }
        catch (const std::exception & e)
        {
            std::cerr << "Build error: " << e.what() << std::endl;
            return 1;
        }
    }
    
    std::vector<fs::path> EuropaBuild::discoverSourceFiles()
    {
        std::vector<fs::path> source_files;

        for (const auto & entry : fs::recursive_directory_iterator(config_.source_dir))
        {
            if (entry.is_regular_file()) {
                const auto & path = entry.path();
                const auto ext = path.extension();

                if (ext == ".cpp" || ext == ".cxx" || ext == ".cc" || ext == ".c++")
                {
                    source_files.push_back(fs::relative(path, config_.source_dir));
                }
            }
        }

        std::sort(source_files.begin(), source_files.end());
        return source_files;
    }
    
    std::unique_ptr<MPP::Compiler> EuropaBuild::detectCompiler()
    {
        const std::vector<std::string> compilers = { "g++", "clang++", "c++" };

        for (const auto & compiler_name : compilers)
        {
            auto compiler = MPP::Compilers::detect_compiler(MPP::ELanguage::CPP, MPP::EMachine::BUILD, {compiler_name});
            if (compiler)
            {
                return compiler;
            }
        }

        return nullptr;
    }
    
    void EuropaBuild::generateNinjaBuild(const std::vector<fs::path> & source_files,
                                             MPP::Compiler * compiler)
    {
        std::ofstream out(config_.build_dir / "build.ninja");

        out << "# EuropaBuild C++ Build System" << std::endl
            << "# Generated build file" << std::endl
            << std::endl
            << "ninja_required_version = 1.8.2" << std::endl
            << std::endl;

        // Write compiler rule
        writeCompilerRule(out, compiler);

        // Write linker/archiver rules
        if (config_.build_type == "exe")
        {
            writeLinkerRule(out, compiler);
        }
        else
        {
            writeArchiverRule(out, compiler);
        }

        // Write build rules for each source file
        std::vector<std::string> object_files;
        for (const auto & source_file : source_files)
        {
            std::string obj_file = source_file.stem().string() + ".o";
            object_files.push_back(obj_file);

            out << "build " << obj_file << ": cpp_compile " << source_file << std::endl;
            out << "  ARGS =";
            for (const auto & arg : config_.cpp_args)
            {
                out << " " << arg;
            }
            out << std::endl << std::endl;
        }

        // Write final target rule
        if (config_.build_type == "exe")
        {
            out << "build " << config_.output_name << ": cpp_link";
            for (const auto & obj : object_files)
            {
                out << " " << obj;
            }
            out << std::endl << std::endl;
        }
        else
        {
            out << "build " << config_.output_name << ": cpp_archive";
            for (const auto & obj : object_files)
            {
                out << " " << obj;
            }
            out << std::endl << std::endl;
        }

        out << "default " << config_.output_name << std::endl;
    }
    
    void EuropaBuild::writeCompilerRule(std::ofstream & out, MPP::Compiler * compiler)
    {
        out << "rule cpp_compile" << std::endl << "  command =";

        for (const auto & cmd : compiler->command)
        {
            out << " " << cmd;
        }

        // add compile-only and output commands
        auto compile_cmd = compiler->compile_only_command();
        for (const auto & arg : compile_cmd)
        {
            out << " " << arg;
        }

        out << " ${ARGS} -o ${out} ${in}" << std::endl
            << "  description = Compiling C++ object ${out}" << std::endl
            << std::endl;
    }
    
    void EuropaBuild::writeLinkerRule(std::ofstream & out, MPP::Compiler * compiler)
    {
        out << "rule cpp_link" << std::endl << "  command =";

        for (const auto & cmd : compiler->command)
        {
            out << " " << cmd;
        }

        out << " ${ARGS} -o ${out} ${in}" << std::endl
            << "  description = Linking executable ${out}" << std::endl
            << std::endl;
    }
    
    void EuropaBuild::writeArchiverRule(std::ofstream & out, MPP::Compiler * compiler)
    {
        out << "rule cpp_archive" << std::endl
            << "  command = ar rcs ${out} ${in}" << std::endl
            << "  description = Creating static library ${out}" << std::endl
            << std::endl;
    }
    
    BuildConfig parse_arguments(int argc, char * argv[])
    {
        BuildConfig config;
        config.source_dir = fs::current_path();
        config.build_dir = fs::current_path() / "build";
        config.output_name = "app";
        config.build_type = "exe";

        for (int i = 1; i < argc; i++)
        {
            std::string arg = argv[i];

            if (arg == "--help" || arg == "-h")
            {
                std::cout << "Simple C++ Build System" << std::endl
                          << "Usage: " << argv[0] << " [options]" << std::endl
                          << "Options:" << std::endl
                          << "  --source-dir DIR    Source directory (default: current directory)"
                          << std::endl
                          << "  --build-dir DIR     Build directory (default: ./build)" << std::endl
                          << "  --output NAME       Output name (default: app)" << std::endl
                          << "  --type TYPE         Build type: exe or lib (default: exe)" << std::endl
                          << "  --cpp-arg ARG       Additional C++ compiler arguments" << std::endl
                          << "  --verbose           Verbose output" << std::endl
                          << "  --help, -h          Show this help" << std::endl;
                exit(0);
            }
            else if (arg == "--source-dir" && i + 1 < argc)
            {
                config.source_dir = fs::absolute(argv[++i]);
            }
            else if (arg == "--build-dir" && i + 1 < argc)
            {
                config.build_dir = fs::absolute(argv[++i]);
            }
            else if (arg == "--output" && i + 1 < argc)
            {
                config.output_name = argv[++i];
            }
            else if (arg == "--type" && i + 1 < argc)
            {
                config.build_type = argv[++i];
                if (config.build_type != "exe" && config.build_type != "lib") {
                    std::cerr << "Invalid build type. Must be 'exe' or 'lib'" << std::endl;
                    exit(1);
                }
            }
            else if (arg == "--cpp-arg" && i + 1 < argc)
            {
                config.cpp_args.push_back(argv[++i]);
            }
            else if (arg == "--verbose")
            {
                config.verbose = true;
            }
            else
            {
                std::cerr << "Unknown argument: " << arg << std::endl;
                exit(1);
            }
        }

        return config;
    }
    
}

int main(int argc, char* argv[]) 
{
    try 
    {
        auto config = EuropaBuild::parse_arguments(argc, argv);
        EuropaBuild::EuropaBuild builder(config);
        return builder.build();
    } 
    catch (const std::exception& e) 
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

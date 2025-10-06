
#pragma once
#include <filesystem>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>

#include "mpp_fdecl.hpp"

namespace EuropaBuild 
{
    namespace fs = std::filesystem;

    struct BuildConfig 
    {
        fs::path source_dir;
        fs::path build_dir;
        std::string output_name;
        std::string build_type; // "exe" or "lib"
        std::vector<std::string> cpp_args;
        bool verbose = false;
    };

    class EuropaBuild
    {
    public:
		EuropaBuild(const BuildConfig& config);
        
        int build();
    
    private:
        BuildConfig config_;
    
        std::vector<fs::path> discoverSourceFiles();
    
        std::unique_ptr<MPP::Compiler> detectCompiler();
    
        void generateNinjaBuild(const std::vector<fs::path>& source_files, MPP::Compiler* compiler);
    
        void writeCompilerRule(std::ofstream& out, MPP::Compiler* compiler);
    
        void writeLinkerRule(std::ofstream & out, MPP::Compiler * compiler);
    
        void writeArchiverRule(std::ofstream& out, MPP::Compiler* compiler);
    };

    BuildConfig parse_arguments(int argc, char* argv[]);
    
} // namespace SimpleBuild

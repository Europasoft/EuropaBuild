
#pragma once
#include <config_types.hpp>
//#include "mpp_fdecl.hpp"

#include <filesystem>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <memory>
#include "config.hpp"

namespace EuropaBuild 
{
	namespace fs = std::filesystem;

	struct TargetMapping
	{
		std::shared_ptr<const Target> target = nullptr;
		std::vector<fs::path> sourceFiles;
	};

	class BuildTool
	{
	public:
		BuildTool(std::shared_ptr<const BuildConfig2> config);
		
		int build();

		ESLogVerbosity getLogVerbosity() const;
	
	private:
		std::shared_ptr<const BuildConfig2> _config;

		ESLog log;
	
		std::vector<fs::path> discoverSourceFiles(const std::vector<fs::path>& paths);
	
		//std::unique_ptr<MPP::Compiler> detectCompiler();
	
		//void generateNinjaBuild(std::shared_ptr<std::vector<TargetMapping>> mappings, MPP::Compiler* compiler);
	
		//void writeCompilerRule(std::ofstream& out, MPP::Compiler* compiler);
	
		//void writeLinkerRule(std::ofstream & out, MPP::Compiler * compiler);
	
		//void writeArchiverRule(std::ofstream& out, MPP::Compiler* compiler);

		//std::string sourceFilePathToString(const fs::path& path);

		int8_t runNinja();

		static void createRelativeDirectory(const fs::path& path);
	};


} // namespace EuropaBuild

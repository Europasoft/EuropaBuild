
#pragma once
#include <EuropaBuild/config_types.hpp>
#include "EuropaBuild/mpp.hpp"
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

		int8_t runNinja();

		static void createRelativeDirectory(const fs::path& path);

		static std::unique_ptr<MPP::Compiler> detectCompiler();

		static void writeCompilerRule(std::ofstream& out, MPP::Compiler* compiler);

		static void writeLinkerRule(std::ofstream& out, MPP::Compiler* compiler);

		static void writeArchiverRule(std::ofstream& out, MPP::Compiler* compiler);

		static fs::path escapeSpacesForNinja(const fs::path& p);

		static std::string sourceFilePathToObjFilenameString(const fs::path& objOutDir, const fs::path& sourcePath, std::string suffix);

		static std::string includePathArgs(const TargetMapping& mapping);

		static void generateNinjaBuild(const BuildConfig2& _config, std::shared_ptr<std::vector<TargetMapping>> mappings, MPP::Compiler* compiler);

	};


} // namespace EuropaBuild

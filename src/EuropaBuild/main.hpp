
#pragma once
#include <EuropaBuild/config_types.hpp>

#include <filesystem>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <memory>
#include "config.hpp"

namespace EuropaBuild::FindTool
{
	class Compiler;
	class Toolchain;
}

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
		BuildTool(std::shared_ptr<const BuildConfig> config);
		
		int build();

		ESLogVerbosity getLogVerbosity() const;
	
	private:
		std::shared_ptr<const BuildConfig> _config;

		ESLog log;
	
		std::vector<fs::path> discoverSourceFiles(const std::vector<fs::path>& paths);

		int8_t runNinja();

		static void createRelativeDirectory(const fs::path& path);

		static void writeCompilerRule(std::ofstream& out, std::shared_ptr<FindTool::Toolchain> toolchain);

		static void writeLinkerRule(std::ofstream& out, std::shared_ptr<FindTool::Toolchain> toolchain);

		static void writeArchiverRule(std::ofstream& out, std::shared_ptr<FindTool::Toolchain> toolchain);

		static fs::path escapeSpacesForNinja(const fs::path& p);

		static std::string sourceFilePathToObjFilenameString(const fs::path& objOutDir, const fs::path& sourcePath, std::string suffix);

		static std::string includePathArgs(const TargetMapping& mapping);

		static std::string makeTargetFullOutputPath(const Target& target);

		static void generateNinjaBuild(const BuildConfig& _config, std::shared_ptr<std::vector<TargetMapping>> mappings, 
							std::shared_ptr<FindTool::Toolchain> toolchain);

	};

	static constexpr bool WinOS =
#if defined(_WIN32) || defined(_WIN64)
		true;
#else
		false;
#endif

} // namespace EuropaBuild

#pragma once

#include <filesystem>
#include <vector>
#include <string>
#include <memory>

namespace EuropaBuild::FindTool
{
	class Compiler;
	class Toolchain;
}

namespace EuropaBuild
{
	namespace fs = std::filesystem;

	class BuildTree;
	struct TargetMapping;
	struct Target;

	class BuildTool
	{
	public:
		BuildTool(std::shared_ptr<const BuildTree> treePtr);

		int build();

	private:
		std::shared_ptr<const BuildTree> tree;

		std::vector<fs::path> discoverSourceFiles(const std::vector<fs::path>& paths);

		int8_t runNinja();

		static void createRelativeDirectory(const fs::path& path);

		static void writeCompilerRule(std::ofstream& out, std::shared_ptr<FindTool::Toolchain> toolchain);

		static void writeLinkerRule(std::ofstream& out, std::shared_ptr<FindTool::Toolchain> toolchain);

		static void writeArchiverRule(std::ofstream& out, std::shared_ptr<FindTool::Toolchain> toolchain);

		static fs::path escapeSpacesForNinja(const fs::path& p);

		static std::string sourceFilePathToObjFilenameString(const fs::path& sourcePath, std::string suffix);

		static std::string includePathArgs(const TargetMapping& mapping);

		static std::string makeTargetFullOutputPath(const Target& target);

		static void generateNinjaBuild(const BuildTree& tree, std::shared_ptr<std::vector<TargetMapping>> mappings,
			std::shared_ptr<FindTool::Toolchain> toolchain);

	};

} // namespace EuropaBuild
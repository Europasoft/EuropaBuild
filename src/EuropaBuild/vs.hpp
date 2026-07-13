#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <memory>

namespace EuropaBuild
{
	class BuildTree;
	struct Target;
}

namespace EuropaBuild::VS
{
	namespace fs = std::filesystem;
	using namespace EuropaBuild;

	class VSParser
	{
	public:
		// parse a .sln file and all its referenced .vcxproj files
		static std::shared_ptr<BuildTree> parseSolution(const fs::path& slnPath);

		// parse a single .vcxproj file
		static std::shared_ptr<Target> parseProject(const fs::path& vcxprojPath, const fs::path& solutionDir = "");

	private:
		// helper to expand MSBuild macros like $(ProjectDir)
		static std::string expandMacros(std::string str, const fs::path& projectDir, const fs::path& solutionDir);
	};
}
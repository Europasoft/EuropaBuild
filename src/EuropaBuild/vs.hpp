#pragma once
#include "EuropaBuild/config_types.hpp"
#include <filesystem>
#include <string>
#include <vector>
#include <memory>

namespace EuropaBuild::VS
{
	namespace fs = std::filesystem;

	class VSParser
	{
	public:
		// parse a .sln file and all its referenced .vcxproj files
		static std::shared_ptr<BuildConfig> parseSolution(const fs::path& slnPath);

		// parse a single .vcxproj file
		static std::shared_ptr<Target> parseProject(const fs::path& vcxprojPath, const fs::path& solutionDir = "");

	private:
		// helper to expand MSBuild macros like $(ProjectDir)
		static std::string expandMacros(std::string str, const fs::path& projectDir, const fs::path& solutionDir);
	};
}
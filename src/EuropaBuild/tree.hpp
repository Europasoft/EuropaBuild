#pragma once

#include <filesystem>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <memory>
#include <utility>
#include <cstdint>
#include <string_view>


namespace EuropaBuild
{
	namespace fs = std::filesystem;

	enum class ETargetType : int
	{
		Unknown = 0,
		Dependency = 1,
		Executable = 2,
		StaticLib = 3,
		DynamicLib = 4,
	};

	struct Target
	{
		std::string name;
		ETargetType targetType = ETargetType::Executable;
		fs::path outputPath;
		std::vector<fs::path> sources;
		std::vector<fs::path> includePaths;
		std::vector<std::string> depends;
		std::vector<fs::path> libPaths;
		std::vector<std::string> libs;

		// standard names for fields in a target JSON object
		static inline constexpr auto NAME = "target";
		static inline constexpr auto TYPE = "type";
		static inline constexpr auto OUTPATH = "output path";
		static inline constexpr auto SOURCES = "sources";
		static inline constexpr auto INCLUDES = "include paths";
		static inline constexpr auto DEPENDS = "dependencies";
		static inline constexpr auto LIBPATHS = "lib paths";
		static inline constexpr auto LIBS = "libs";
	};

	struct TargetMapping
	{
		std::shared_ptr<const Target> target = nullptr;
		std::vector<fs::path> sourceFiles;
	};

	using Targets = std::vector<std::shared_ptr<const Target>>;

	struct GeneralBuildSettings
	{
		// if a json object has a subobject with this name, treat it as general settings instead of a build target definition
		static inline constexpr auto NAME = "general settings";
		static inline constexpr auto CPP_COMP_ARGS = "c++ compiler args";
		std::vector<std::string> cppCompilerArgs;
		static inline constexpr auto C_COMP_ARGS = "c compiler args";
		std::vector<std::string> cCompilerArgs;
		static inline constexpr auto LNK_ARGS = "linker args";
		std::vector<std::string> linkerArgs;
	};

	// the build tree orders the targets into a sensible build order based on their interdependencies
	class BuildTree
	{
		Targets targetsOrdered;
	public:
		Targets targetsThatAreFinalProducts;
		GeneralBuildSettings generalSettings;
		BuildTree(const Targets& targets, GeneralBuildSettings generalSettings);

		Targets::const_iterator begin() const;
		Targets::const_iterator end() const;
		size_t size() const;
		std::string getWholeDependecyTreeAsString() const;
	private:
		std::string getDependecyTreeForTarget(const Target& t, size_t& iterationDepth) const;
	};

} // namespace EuropaBuild


#pragma once
#include "europasoft-json/Source/Parser.h"

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

	struct ConfigException : std::runtime_error
	{
		using std::runtime_error::runtime_error;
	};
	struct DependencyException : std::runtime_error
	{
		using std::runtime_error::runtime_error;
	};
	struct EnvironmentException : std::runtime_error
	{
		using std::runtime_error::runtime_error;
	};

	enum class ETargetType : uint32_t
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

		// standard names for fields in a target JSON object
		static inline constexpr auto NAME = "target";
		static inline constexpr auto TYPE = "type";
		static inline constexpr auto OUTPATH = "output path";
		static inline constexpr auto SOURCES = "sources";
		static inline constexpr auto INCLUDES = "include paths";
		static inline constexpr auto DEPENDS = "dependencies";
	};

	using Targets = std::vector<std::shared_ptr<const Target>>;

	// the build tree orders the targets into a sensible build order based on their interdependencies
	class BuildTree
	{
		Targets targetsOrdered;
	public:
		Targets targetsThatAreFinalProducts;
		BuildTree(const Targets& targets);

		Targets::const_iterator begin() const;
		Targets::const_iterator end() const;
		size_t size() const;
		std::string getWholeDependecyTreeAsString() const;
	private:
		std::string getDependecyTreeForTarget(const Target& t, size_t& iterationDepth) const;
	};

	static inline constexpr auto INTERMEDIATE_DIR = "Intermediate";

	class BuildConfig
	{
	public:
		std::unique_ptr<BuildTree> tree = nullptr;
		fs::path intermediateDir;
	};

} // namespace EuropaBuild

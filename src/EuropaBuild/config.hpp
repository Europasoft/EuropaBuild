#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace JSON 
{
	class Object;
	using ObjectPtr = std::shared_ptr<Object>;
}

namespace EuropaBuild
{
	class BuildTree;
	enum class ETargetType : int;
	namespace fs = std::filesystem;

	class ConfigUtils
	{
	public:
		static std::shared_ptr<BuildTree> parseBuildTreeFromJson(const std::filesystem::path& fullPath);
		//static BuildConfig parseConfigFromCommands(int argc, char* argv[]);
		static void targetSanityCheck(const JSON::ObjectPtr& targetJson);
		static ETargetType targetTypeFromString(std::string_view str);
		static std::vector<fs::path> vectorStringsToPaths(std::vector<std::string> strs);
		static fs::path sanitizeStringToPath(std::string_view sv);
		static std::string lowercase(std::string_view sv);
		static void cleanIntermediateFiles();
	};

	class EuropaBuildArgs
	{
	public:
		// defaults - configurable through start arguments
		fs::path configPath;
		bool rebuild = false;

		EuropaBuildArgs(int argc, char* argv[]);
	};

} // namespace EuropaBuild

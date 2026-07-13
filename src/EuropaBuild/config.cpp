#include "EuropaBuild/config.hpp"
#include "EuropaBuild/tree.hpp"
#include "EuropaBuild/main.hpp"
#include "europasoft-json/Source/Parser.h"

#include <cstdint>
#include <stdexcept>
#include <queue>
#include <unordered_map>
#include <set>
#include <cctype>
#include <memory>
#include <string>
#include <map>

namespace fs = std::filesystem;

namespace EuropaBuild
{
	std::shared_ptr<BuildTree> ConfigUtils::parseBuildTreeFromJson(const std::filesystem::path& fullPath)
	{
		using namespace JSON;
		
		JSON::Object root;
		JSON::Result jsonResult = JSON::loadFromFile(fullPath.string(), root);
		if (jsonResult != JSON::Result::OK)
		{
			auto jsonError = static_cast<uint32_t>(jsonResult);
			const auto errorCategory = (jsonError >= 300) ? " (filesystem)" : ((jsonError < 200) ? " (lexer)" : " (parser)");
			throw ConfigException("Failed to process the JSON build file. Error code " + std::to_string(jsonError) + errorCategory);
		}

		if (root.size() != 1 or not root.isContainer())
			throw ConfigException("The JSON build file appears to be empty");

		const JSON::Object& file = root[0];
		if (not file.isArray())
			throw ConfigException("The root of the build file must be a JSON array");
		if (file.size() < 1)
			throw ConfigException("No rules found in the build file");

		// process each target specified in the build file
		std::vector<std::shared_ptr<const Target>> targets;
		for (const JSON::ObjectPtr& targetJson : file)
		{
			targetSanityCheck(targetJson);
			std::shared_ptr<Target> targetPtr = std::make_shared<Target>();
			Target& target = *targetPtr;
			std::map<std::string, JSON::ObjectPtr> targetFields = targetJson->map();

			target.name =			targetFields[Target::NAME]->getValue();
			target.targetType =		targetTypeFromString(targetFields[Target::TYPE]->getValue());
			target.outputPath =		sanitizeStringToPath(targetFields[Target::OUTPATH]->getValue());
			target.sources =		vectorStringsToPaths(targetFields[Target::SOURCES]->vector());
			target.includePaths =	vectorStringsToPaths(targetFields[Target::INCLUDES]->vector());
			target.depends =		targetFields[Target::DEPENDS]->vector();
			// read optional library parameters (or set empty arrays if absent)
			if (targetJson->hasNamedSubobject(Target::LIBPATHS))
			{
				target.libPaths = vectorStringsToPaths(targetFields[Target::LIBPATHS]->vector());
			}
			if (targetJson->hasNamedSubobject(Target::LIBS))
			{
				target.libs = targetFields[Target::LIBS]->vector();
			}

			targets.push_back(targetPtr);
		}

		// this performs dependency analysis to sort the targets
		std::shared_ptr<BuildTree> tree = std::make_shared<BuildTree>(targets);
		return tree;
	}

	ETargetType ConfigUtils::targetTypeFromString(std::string_view str)
	{
		if (str == "executable" or str == "exe")
			return ETargetType::Executable;
		else if (str == "dependency" or str == "dep")
			return ETargetType::Dependency;
		else if (str == "static library" or str == "static lib")
			return ETargetType::StaticLib;
		else if (str == "dynamic library" or str == "dynamic lib" or str == "dll")
			return ETargetType::DynamicLib;

		return ETargetType::Unknown;
	}
	
#define SANITY_CHECK_FIELD(targetJson, field) if (not targetJson->hasNamedSubobject(field))\
	throw ConfigException("Target is missing a required field: " + std::string(field) + "\n" + targetJson->toString(true));

	void ConfigUtils::targetSanityCheck(const JSON::ObjectPtr& targetJson)
	{
		SANITY_CHECK_FIELD(targetJson, Target::NAME);
		SANITY_CHECK_FIELD(targetJson, Target::TYPE);
		SANITY_CHECK_FIELD(targetJson, Target::OUTPATH);
		SANITY_CHECK_FIELD(targetJson, Target::SOURCES);

		const auto src = targetJson->getNamedSubobject(Target::SOURCES);
		if ((not src->isArray()) or src->size() < 1)
		{
			throw ConfigException("Target needs an array of sources: " + targetJson->toString(true));
		}
	}

	std::vector<fs::path> ConfigUtils::vectorStringsToPaths(std::vector<std::string> sts)
	{
		std::vector<fs::path> paths;
		for (const auto& s : sts)
			paths.push_back(sanitizeStringToPath(s));
		return paths;
	}

	fs::path ConfigUtils::sanitizeStringToPath(std::string_view sv)
	{
		std::string s(sv);
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch)
			{
				return !std::isspace(ch);
			}));
		s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch)
			{
				return !std::isspace(ch);
			}).base(), s.end());
		
		std::replace(s.begin(), s.end(), '\\', '/');
		return fs::path(s);
	}

	std::string ConfigUtils::lowercase(std::string_view sv)
	{
		std::string s(sv);
		std::transform(s.begin(), s.end(), s.begin(),
			[](unsigned char c)
			{
				return std::tolower(c);
			});
		return s;
	}

	void ConfigUtils::cleanIntermediateFiles()
	{
		fs::path intermediateDir = GET_INTERMEDIATE_PATH();
		if (fs::exists(intermediateDir) && fs::is_directory(intermediateDir))
		{
			log("Clean rebuild\n");
			// wipe *.o files
			for (const auto& entry : fs::directory_iterator(intermediateDir))
			{
				if (entry.is_regular_file() && entry.path().extension() == ".o")
				{
					fs::remove(entry.path());
				}
			}
		}
		else
		{
			log("Could not find intermediate files - cleaning skipped");
		}
	}


	EuropaBuildArgs::EuropaBuildArgs(int argc, char* argv[])
	{
		log("\n" + std::string(SPLASH_MESSAGE) + "\n\n", LogColors::CYAN);
		// fallback to default path if not provided
		configPath = std::filesystem::current_path() / CONF_FILENAME;

		for (int i = 1; i < argc; ++i)
		{
			std::string_view arg(argv[i]);

			if (arg == "-r" || arg == "--rebuild")
			{
				rebuild = true;
			}
			// -c or --config to use an alternate path to the build config json file
			else if (arg == "-c" || arg == "--config")
			{
				if (i + 1 < argc)
				{
					configPath = argv[++i]; // consume next arg as the path
				}
				else
				{
					throw ConfigException("Missing path after " + std::string(arg));
				}
			}
			else
			{
				throw ConfigException("Unknown argument: " + std::string(arg));
			}
		}
	}

} // namespace EuropaBuild



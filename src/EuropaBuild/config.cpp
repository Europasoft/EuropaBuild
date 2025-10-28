
#include "EuropaBuild/config.hpp"
#include "json/Parser.h"

#include <cstdint>
#include <stdexcept>
#include <queue>
#include <unordered_map>
#include <set>
#include <cctype>
#include <memory>
#include <string>
#include <map>

namespace EuropaBuild
{
	/*BuildConfig BuildConfig::configDefault()
	{
		BuildConfig config;
		config.source_dir = fs::current_path();
		config.build_dir = fs::current_path() / "build";
		config.output_name = "app";
		config.build_type = "exe";
		return config;
	}*/

	std::shared_ptr<BuildConfig2> ConfigUtils::parseConfigFromJson(const std::filesystem::path& fullPath)
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
			
			targets.push_back(targetPtr);
		}

		std::shared_ptr<BuildConfig2> config = std::make_shared<BuildConfig2>();
		config->tree = std::make_unique<BuildTree>(targets); // this performs dependency analysis to sort the targets

		return config;
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
	
	/*BuildConfig parseConfigFromCommands(int argc, char* argv[])
	{
		BuildConfig config = BuildConfig::configDefault();

		for (int i = 1; i < argc; i++)
		{
			std::string arg = argv[i];

			if (arg == "--help" || arg == "-h")
			{
				std::cout << "Simple C++ Build System" << std::endl
					<< "Usage: " << argv[0] << " [options]" << std::endl
					<< "Options:" << std::endl
					<< "  --source-dir DIR    Source directory (default: current directory)"
					<< std::endl
					<< "  --build-dir DIR     Build directory (default: ./build)" << std::endl
					<< "  --output NAME       Output name (default: app)" << std::endl
					<< "  --type TYPE         Build type: exe or lib (default: exe)" << std::endl
					<< "  --cpp-arg ARG       Additional C++ compiler arguments" << std::endl
					<< "  --verbose           Verbose output" << std::endl
					<< "  --help, -h          Show this help" << std::endl;
				exit(0);
			}
			else if (arg == "--source-dir" && i + 1 < argc)
			{
				config.source_dir = fs::absolute(argv[++i]);
			}
			else if (arg == "--build-dir" && i + 1 < argc)
			{
				config.build_dir = fs::absolute(argv[++i]);
			}
			else if (arg == "--output" && i + 1 < argc)
			{
				config.output_name = argv[++i];
			}
			else if (arg == "--type" && i + 1 < argc)
			{
				config.build_type = argv[++i];
				if (config.build_type != "exe" && config.build_type != "lib") {
					std::cerr << "Invalid build type. Must be 'exe' or 'lib'" << std::endl;
					exit(1);
				}
			}
			else if (arg == "--cpp-arg" && i + 1 < argc)
			{
				config.cpp_args.push_back(argv[++i]);
			}
			else if (arg == "--verbose")
			{
				config.verbose = true;
			}
			else
			{
				std::cerr << "Unknown argument: " << arg << std::endl;
				exit(1);
			}
		}

		return config;
	}*/
	
	
	BuildTree::BuildTree(const Targets& targets)
	{
		// sort target dependencies so they will be built in order

		using TargetPtr = std::shared_ptr<const Target>;
		std::unordered_map<std::string, int> inDegree;
		std::unordered_map<std::string, TargetPtr> nameToTarget;
		// maps a dependency to the targets that depend on it (dependency -> dependents)
		std::unordered_map<std::string, std::vector<std::string>> adjacencyList;
		// this is only used to determine which targets are "final", i.e. ones that no others depend on
		std::vector<std::string> allDependencies;

		for (const TargetPtr& target : targets)
		{
			nameToTarget[target->name] = target;
			inDegree[target->name] = 0;
			allDependencies.insert(allDependencies.end(), target->depends.begin(), target->depends.end());
		}

		// build adjacency list and in-degrees
		for (const TargetPtr& target : targets)
		{
			// U is the current target
			const std::string& U = target->name;

			for (const std::string& V : target->depends) 
			{
				// V is a dependency of U, so V must be built before U
				if (nameToTarget.find(V) == nameToTarget.end()) 
				{
					throw std::runtime_error("Failed to resolve dependency graph, target " + U + " depends on non-existent target " + V);
				}

				// V is a prerequisite, and U is a dependent of V
				// add U to V's adjacency list (what V needs to notify when built)
				adjacencyList[V].push_back(U);

				// increment U's in-degree because V is a dependency for U
				inDegree[U]++;
			}
		}

		// Kahn's algorithm
		std::queue<std::string> q;

		for (const auto& pair : inDegree) 
		{
			if (pair.second == 0)
			{
				q.push(pair.first);
			}
		}

		while (!q.empty()) 
		{
			std::string current_name = q.front();
			q.pop();

			targetsOrdered.push_back(nameToTarget.at(current_name));

			// iterate all targets that depend on the current one
			if (adjacencyList.count(current_name)) 
			{
				for (const std::string& dependent_name : adjacencyList.at(current_name)) 
				{
					inDegree[dependent_name]--;

					// if in-degree hits 0 it means all dependencies are satisfied
					if (inDegree[dependent_name] == 0) {
						q.push(dependent_name);
					}
				}
			}
		}

		if (targetsOrdered.size() != targets.size()) 
		{
			throw std::runtime_error("Failed to resolve dependency graph, circular dependency detected");
		}

		for (const TargetPtr& target : targetsOrdered)
		{
			auto it = std::find(allDependencies.begin(), allDependencies.end(), target->name);
			if (it == allDependencies.end())
				targetsThatAreFinalProducts.push_back(target);
		}
	}

	Targets::const_iterator BuildTree::begin() const { return targetsOrdered.begin(); }
	Targets::const_iterator BuildTree::end() const { return targetsOrdered.end(); }
	size_t BuildTree::size() const { return targetsOrdered.size(); }

	std::string BuildTree::getWholeDependecyTreeAsString() const
	{
		std::string s;
		size_t iterationDepth = 0;
		for (const auto& target : targetsOrdered)
			s += "Target: " + getDependecyTreeForTarget(*target, iterationDepth) + "\n";
		return s;
	}

	std::string BuildTree::getDependecyTreeForTarget(const Target& t, size_t& iterationDepth) const
	{
		iterationDepth++;
		std::string s;
		for (auto i = 0; i < iterationDepth - 1; i++)
			s += (i != iterationDepth - 2) ? "  " : " |"; // indent accorrding to dependency nesting level
		s +=  t.name + "\n";
		for (const std::string& d : t.depends)
		{
			Targets::const_iterator it = std::find_if(targetsOrdered.begin(), targetsOrdered.end(),
				[d](const std::shared_ptr<const Target>& candidate)
				{ return candidate->name == d; });
			s += (it != targetsOrdered.end()) ? getDependecyTreeForTarget(*it->get(), iterationDepth) : "???";	
		}
		iterationDepth--;
		return s;
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

} // namespace EuropaBuild



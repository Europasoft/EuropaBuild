#include "EuropaBuild/vs.hpp"
#include "EuropaBuild/main.hpp"
#include "EuropaBuild/tree.hpp"
#include "europasoft-json/Source/Parser.h"
#include <fstream>
#include <regex>
#include <iostream>
#include <sstream>
#include <algorithm>

namespace EuropaBuild::VS
{
	using Target = EuropaBuild::Target;

	std::shared_ptr<BuildTree> VSParser::parseSolution(const fs::path& slnPath)
	{
		if (!fs::exists(slnPath))
		{
			throw ConfigException("Solution file not found: " + slnPath.string());
		}

		const fs::path solutionDir = slnPath.parent_path();
		std::vector<std::shared_ptr<const Target>> targets;

		std::ifstream file(slnPath);
		std::string line;
		// matches: Project("{GUID}") = "ProjectName", "Path\To\Project.vcxproj", "{GUID}"
		std::regex projectRegex(R"vs(Project\("\{[A-F0-9-]+\}"\) = "([^"]+)", "([^"]+\.vcxproj)", "(\{[A-F0-9-]+\})")vs", std::regex::icase);

		while (std::getline(file, line))
		{
			std::smatch match;
			if (std::regex_search(line, match, projectRegex))
			{
				std::string projectName = match[1];
				std::string relativeProjectPath = match[2];

				fs::path vcxprojPath = solutionDir / relativeProjectPath;
				if (fs::exists(vcxprojPath))
				{
					try
					{
						auto target = parseProject(vcxprojPath, solutionDir);
						if (target)
						{
							targets.push_back(target);
						}
					}
					catch (const std::exception& e)
					{
						std::cerr << "Warning: Failed to parse project " << projectName << ": " << e.what() << std::endl;
					}
				}
			}
		}

		auto tree = std::make_shared<BuildTree>(targets, GeneralBuildSettings());
		return tree;
	}

	std::shared_ptr<Target> VSParser::parseProject(const fs::path& vcxprojPath, const fs::path& solutionDir)
	{
		XML::Node root;
		if (XML::loadFromFile(vcxprojPath.string(), root) != XML::Result::OK)
		{
			return nullptr;
		}

		std::shared_ptr<Target> target = std::make_shared<Target>();
		const fs::path projectDir = vcxprojPath.parent_path();
		target->name = vcxprojPath.stem().string();
		target->outputPath = "build/";

		for (const auto& child : root)
		{
			// extract ConfigurationType (target type)
			if (child->getName() == "PropertyGroup")
			{
				for (const auto& prop : *child)
				{
					if (prop->getName() == "ConfigurationType")
					{
						std::string type = std::string(prop->getValue());
						if (type == "Application") target->targetType = ETargetType::Executable;
						else if (type == "StaticLibrary") target->targetType = ETargetType::StaticLib;
						else if (type == "DynamicLibrary") target->targetType = ETargetType::DynamicLib;
					}
				}
			}
			// extract sources and project references
			else if (child->getName() == "ItemGroup")
			{
				for (const auto& item : *child)
				{
					if (item->getName() == "ClCompile")
					{
						auto attrs = item->getAttributes();
						if (attrs.count("Include"))
						{
							std::string src = expandMacros(attrs.at("Include"), projectDir, solutionDir);
							// add the actual file path instead of just the parent directory
							target->sources.push_back(fs::path(src));
						}
					}
					else if (item->getName() == "ProjectReference")
					{
						auto attrs = item->getAttributes();
						if (attrs.count("Include"))
						{
							// use the stem of the project path as the dependency name
							fs::path depPath = fs::path(attrs.at("Include")).stem();
							target->depends.push_back(depPath.string());
						}
					}
				}
			}
			// extract include directories
			else if (child->getName() == "ItemDefinitionGroup")
			{
				for (const auto& def : *child)
				{
					if (def->getName() == "ClCompile")
					{
						for (const auto& prop : *def)
						{
							if (prop->getName() == "AdditionalIncludeDirectories")
							{
								std::string includes = expandMacros(std::string(prop->getValue()), projectDir, solutionDir);
								std::stringstream ss(includes);
								std::string item;
								while (std::getline(ss, item, ';'))
								{
									if (!item.empty())
										target->includePaths.push_back(fs::path(item));
								}
							}
						}
					}
				}
		}
	}

	// deduplicate source files
	std::sort(target->sources.begin(), target->sources.end());
	target->sources.erase(std::unique(target->sources.begin(), target->sources.end()), target->sources.end());

	return target;
	}

	std::string VSParser::expandMacros(std::string str, const fs::path& projectDir, const fs::path& solutionDir)
	{
		auto replace = [&](const std::string& macro, const std::string& value) {
			size_t pos = 0;
			while ((pos = str.find(macro, pos)) != std::string::npos)
			{
				str.replace(pos, macro.length(), value);
				pos += value.length();
			}
			};

		replace("$(ProjectDir)", projectDir.string() + "/");
		replace("$(SolutionDir)", solutionDir.string() + "/");
		replace("$(Configuration)", "Release"); // default expansion
		replace("$(Platform)", "x64");

		std::replace(str.begin(), str.end(), '\\', '/');
		return str;
	}
}
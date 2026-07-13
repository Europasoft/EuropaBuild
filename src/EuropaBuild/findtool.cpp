
#include "EuropaBuild/findtool.hpp"
#include "util/process.hpp"
#include "europasoft-json/Source/Parser.h"

#include <filesystem>
#include <iostream>
#include <algorithm>
#include <cstdlib>

namespace fs = std::filesystem;

namespace EuropaBuild::FindTool
{
	bool Compiler::isPresent() const
	{
		auto const& [ret, out, err] = Util::process({ this->locateCommand });
		return (ret == 0) and (out.find(this->locateMatch) != std::string::npos);
	}

	std::shared_ptr<Toolchain> Toolchain::selectToolchain()
	{
		std::shared_ptr<Toolchain> toolchain;
		toolchain = std::make_shared<Toolchain>();

		// search for available compilers
		static const std::vector<std::shared_ptr<Compiler>> compilers =
		{
			// TODO: support generating visual studio project files
			//EuropaBuild::MSVC::findMSVC(),

			std::make_shared<Compiler>(Compiler{
				.name = "CLANG",
				.command = "clang++",
				.compileFlag = "-c",
				.locateCommand = "clang++ --version",
				.locateMatch = "clang version"}),

			std::make_shared<Compiler>(Compiler{
				.name = "GNU", 
				.command = "g++", 
				.compileFlag = "-c", 
				.locateCommand = "g++ --version", 
				.locateMatch = "Free Software Foundation"})
		};

		for (const std::shared_ptr<Compiler>& com : compilers)
		{
			if (com.get() and com->isPresent())
			{
				toolchain->compiler = com;
				break; // found a suitable compiler
			}
		}
		
		return toolchain;
	}



} // namespace EuropaBuild

namespace EuropaBuild::MSVC
{

	fs::path findExistingDirPathInList(const std::vector<fs::path>& paths)
	{
		auto it = std::find_if(paths.begin(), paths.end(),
			[&](const fs::path& path) {
				return fs::exists(path) && fs::is_directory(path);
			}
		);
		return (it != paths.end()) ? *it : fs::path();
	}

	fs::path findExistingFilePathInList(const std::vector<fs::path>& paths)
	{
		auto it = std::find_if(paths.begin(), paths.end(),
			[&](const fs::path& path) {
				return fs::exists(path) && fs::is_regular_file(path);
			}
		);
		return (it != paths.end()) ? *it : fs::path();
	}

	std::shared_ptr<Compiler> findMSVC()
	{
		VariablePath p{ std::vector<VariablePathPart>{
				VariablePathPart{std::vector<fs::path>{ "C:\\", "D:\\", "E:\\", "F:\\", "G:\\", "H:\\", "I:\\" }},
				VariablePathPart{std::vector<fs::path>{ "Program Files", "Program Files (x86)", "Programs", "" }},
				VariablePathPart{std::vector<fs::path>{ "Microsoft Visual Studio", "Visual Studio" }}
		}};

		std::cout << "searching for visual studio installation" << "\n";
		for (auto ps : p.getPossiblePaths())
			std::cout << ps << "\n";

		auto found = findExistingDirPathInList(p.getPossiblePaths());
		std::cout << "found visual studio: " << found.string() << "\n";


		//std::make_shared<Compiler>(Compiler{
		//		.name = "CLANG",
		//		.command = "clang++",
		//		.compileFlag = "-c",
		//		.locateCommand = "clang++ --version",
		//		.locateMatch = "clang version" });

		return std::shared_ptr<Compiler>();
	}

	void generatePathsRecursive(
		const std::vector<VariablePathPart>& parts,
		std::vector<fs::path>& result_paths,
		fs::path current_path,
		size_t part_index)
	{
		if (part_index >= parts.size())
		{
			if (!current_path.empty())
			{
				result_paths.push_back(current_path);
			}
			return;
		}

		const VariablePathPart& current_part = parts[part_index];

		for (const fs::path& possibility : current_part.possibilities)
		{
			fs::path next_path = current_path / possibility;
			generatePathsRecursive(
				parts,
				result_paths,
				next_path,
				part_index + 1);
		}
	}

	std::vector<fs::path> VariablePath::getPossiblePaths() const
	{
		std::vector<fs::path> paths;
		if (parts.empty())
		{
			return paths;
		}
		generatePathsRecursive(parts, paths, fs::path(), 0);
		return paths;
	}

} // namespace MSVC



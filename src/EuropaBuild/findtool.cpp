
#include "EuropaBuild/findtool.hpp"
#include "EuropaBuild/compilers.hpp"
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

	bool Compiler::isFileCompatible(const std::filesystem::path& ext, const std::vector<std::string>& acceptable)
	{
		auto x = ext.string();
		std::transform(x.begin(), x.end(), x.begin(), [](unsigned char c)
			{
				return std::tolower(c);
			});
		return std::any_of(acceptable.begin(), acceptable.end(), [&x](const std::string& valid_ext)
			{
				return x == valid_ext;
			});
	}

	std::shared_ptr<Toolchain> Toolchain::selectToolchain()
	{
		std::shared_ptr<Toolchain> toolchain;
		toolchain = std::make_shared<Toolchain>();

		// search for available compilers
		for (const Compiler* com : AllCompilers::cppCompilers)
		{
			if (com and com->isPresent())
			{
				// found a suitable compiler
				toolchain->compiler = std::make_shared<Compiler>(*com);
				break;
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



#pragma once
#include "EuropaBuild/compilers.hpp"
#include "EuropaBuild/findtool.hpp"

namespace EuropaBuild
{
	using namespace FindTool;
	using CompilerPtr = std::shared_ptr<Compiler>;

	inline Compiler AllCompilers::clang_c =
	{
		.name = "CLANG C",
		.command = "clang",
		.compileFlag = "-c",
		.locateCommand = "clang --version",
		.locateMatch = "clang version",
		.compileRuleName = "c_compile",
		.associatedFileExtensions = cSourceFileExtensions,
	};

	inline Compiler AllCompilers::gcc_c =
	{
		.name = "GNU C",
		.command = "gcc",
		.compileFlag = "-c",
		.locateCommand = "gcc --version",
		.locateMatch = "Free Software Foundation",
		.compileRuleName = "c_compile",
		.associatedFileExtensions = cSourceFileExtensions
	};

	inline Compiler AllCompilers::clang_cpp =
	{
		.name = "CLANG",
		.command = "clang++",
		.compileFlag = "-c",
		.locateCommand = "clang++ --version",
		.locateMatch = "clang version",
		.compileRuleName = "cpp_compile",
		.associatedFileExtensions = cppSourceFileExtensions,
		.compiler2 = &AllCompilers::clang_c
	};

	inline Compiler AllCompilers::gcc_cpp =
	{
			.name = "GNU",
			.command = "g++",
			.compileFlag = "-c",
			.locateCommand = "g++ --version",
			.locateMatch = "Free Software Foundation",
			.compileRuleName = "cpp_compile",
			.associatedFileExtensions = cppSourceFileExtensions,
			.compiler2 = &AllCompilers::gcc_c
	};

	inline std::vector<Compiler*> AllCompilers::cppCompilers =
	{
		&AllCompilers::clang_cpp,
		&AllCompilers::gcc_cpp
	};

}


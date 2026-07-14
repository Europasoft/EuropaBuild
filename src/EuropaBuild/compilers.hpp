#pragma once
#include "EuropaBuild/findtool.hpp"

#include <vector>

namespace EuropaBuild
{
	using namespace FindTool;

	struct AllCompilers
	{
		static Compiler clang_c;
		static Compiler gcc_c;
		static Compiler clang_cpp;
		static Compiler gcc_cpp;
		static std::vector<Compiler*> cppCompilers;
	};


}
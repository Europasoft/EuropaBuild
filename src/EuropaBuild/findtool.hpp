
#pragma once
#include <vector>
#include <memory>
#include <string>
#include <string_view>

namespace EuropaBuild::FindTool
{
	class Compiler
	{
	public:
		std::string name;
		std::string command;
		std::string compileFlag;
		std::string locateCommand;
		std::string locateMatch;

		bool isPresent() const;
	};

	class Toolchain
	{
	public:
		std::shared_ptr<Compiler> compiler = nullptr;

		static std::shared_ptr<Toolchain> selectToolchain();
	};

	

} // namespace EuropaBuild

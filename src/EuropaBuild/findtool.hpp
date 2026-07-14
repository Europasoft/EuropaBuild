#pragma once

#include <vector>
#include <memory>
#include <string>
#include <string_view>
#include <filesystem>
#include <functional>

namespace JSON
{
	class Object;
}

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
		std::string compileRuleName;
		std::vector<std::string> associatedFileExtensions;
		Compiler* compiler2 = nullptr;
		bool isPresent() const;
		static bool isFileCompatible(const std::filesystem::path& ext, const std::vector<std::string>& acceptable);
	};

	class Toolchain
	{
	public:
		std::shared_ptr<Compiler> compiler = nullptr;

		static std::shared_ptr<Toolchain> selectToolchain();

	};

	static const std::vector<std::string> cppSourceFileExtensions = { ".cpp", ".cxx", ".cc", ".c++" };
	static const std::vector<std::string> cSourceFileExtensions = { ".c" };

} // namespace EuropaBuild

namespace EuropaBuild::MSVC
{
	using namespace EuropaBuild::FindTool;
	namespace fs = std::filesystem;

	std::shared_ptr<Compiler> findMSVC();

	struct VariablePathPart
	{
		std::vector<fs::path> possibilities;
	};

	struct VariablePath
	{
		std::vector<VariablePathPart> parts;
		std::vector<fs::path> getPossiblePaths() const;
	};

} // namespace EuropaBuild

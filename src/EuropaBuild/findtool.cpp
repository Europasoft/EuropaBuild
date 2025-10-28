
#include "EuropaBuild/findtool.hpp"
#include "util/process.hpp"

namespace EuropaBuild::FindTool
{
	bool Compiler::isPresent() const
	{
		auto const& [ret, out, err] = Util::process({ this->locateCommand });
		return (ret == 0) and (out.find(this->locateMatch) != std::string::npos);
	}

	std::shared_ptr<Toolchain> Toolchain::selectToolchain()
	{
		std::shared_ptr<Toolchain> toolchain = std::make_shared<Toolchain>();

		static const std::vector<std::shared_ptr<Compiler>> compilers =
		{
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
			if (com->isPresent())
				toolchain->compiler = com;
		}
		return toolchain;
	}

} // namespace EuropaBuild



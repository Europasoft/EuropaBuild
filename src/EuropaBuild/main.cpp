#include "EuropaBuild/main.hpp"
#include "EuropaBuild/build.hpp"
#include "EuropaBuild/tree.hpp"
#include "EuropaBuild/config.hpp"
#include "EuropaBuild/findtool.hpp"
#include "util/process.hpp"

#include <filesystem>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <memory>
#include <map>

namespace fs = std::filesystem;

int main(int argc, char* argv[]) 
{
	using namespace EuropaBuild;
    try
    {
		EuropaBuildArgs args(argc, argv);

		// assemble the dependency graph and start the build
		std::shared_ptr<BuildTree> tree = ConfigUtils::parseBuildTreeFromJson(args.configPath);
		if (args.rebuild) ConfigUtils::cleanIntermediateFiles();
        EuropaBuild::BuildTool buildTool(tree);
        return buildTool.build();

    }
    catch (const std::exception& e)
    {
        std::cerr << colorMessage("Error: " + std::string(e.what()), LogColors::BRIGHT_RED);
        return 1;
    }

    std::cout << colorMessage("All done!", LogColors::CYAN);
    return 0;
}

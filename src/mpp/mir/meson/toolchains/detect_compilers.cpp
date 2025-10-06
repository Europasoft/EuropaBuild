// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

/**
 * Compiler detection functions
 */

#include <cassert>
#include <memory>
#include <string>
#include <vector>

// probably not needed - remove later
#ifdef _MSC_VER
#define _SILENCE_CXX20_CONSTRUCT_AT_WARNING
#endif

#include "compiler.hpp"
#include "compilers/cpp/cpp.hpp"
#include "mpp/util/process.hpp"

namespace MIR::Toolchain::Compiler {

namespace {
const std::vector<std::string> DEFAULT_CPP{"c++", "g++", "clang++"};

std::unique_ptr<Compiler> detect_cpp_compiler(const Machines::Machine & m,
                                              const std::vector<std::string> & bins) {
    // TODO: handle the machine switch, and the cross/native file
    for (const auto & c : bins) {
        auto const & [ret, out, err] = Util::process(std::vector<std::string>{c, "--version"});
        if (ret != 0) {
            continue;
        }

        if (out.find("Free Software Foundation") != std::string::npos) {
            return std::make_unique<CPP::Gnu>(std::vector<std::string>{c});
        }
        if (out.find("clang version") != std::string::npos) {
            return std::make_unique<CPP::Clang>(std::vector<std::string>{c});
        }
    }
    return nullptr;
}

} // namespace

std::unique_ptr<Compiler> detect_compiler(const Language & lang, const Machines::Machine & machine,
                                          const std::vector<std::string> & bins) {
    switch (lang) {
        case Language::CPP:
            return detect_cpp_compiler(machine, bins.empty() ? DEFAULT_CPP : bins);
    }
    assert(false);
};

} // namespace MIR::Toolchain::Compiler

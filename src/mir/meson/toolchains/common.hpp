// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

/* Common helpers for toolchains
 */

#pragma once

#include <string>

namespace MIR::Toolchain {

/**
 * If the tool (compiler, linker, archiver) support response files, and which
 * dialect they use.
 */
enum class RSPFileSupport {
    NONE,
    MSVC,
    GCC,
};

/**
 * The toolchain language
 */
enum class Language : unsigned int {
    CPP,
};

Language from_string(const std::string &);

std::string to_string(const Language &);

} // namespace MIR::Toolchain

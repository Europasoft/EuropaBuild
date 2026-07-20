#pragma once
#include "mir/meson/toolchains/compilers/cpp/cpp.hpp"
#include "mir/meson/toolchains/toolchain.hpp"
#include "mir/meson/machines.hpp"
#include "util/log.hpp"
#include "util/exceptions.hpp"
#include "util/process.hpp"

namespace MPP
{
	using Util::process;

	using Compiler = MIR::Toolchain::Compiler::Compiler;
	using MIR::Toolchain::Compiler::detect_compiler;
	using ELanguage = MIR::Toolchain::Language;
	using EMachine = MIR::Machines::Machine;

} // namespace MPP

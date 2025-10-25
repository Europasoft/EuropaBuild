#pragma once
#include "mpp/mir/meson/toolchains/compilers/cpp/cpp.hpp"
#include "mpp/mir/meson/toolchains/toolchain.hpp"
#include "mpp/mir/meson/machines.hpp"
#include "mpp/util/log.hpp"
#include "mpp/util/exceptions.hpp"
#include "mpp/util/process.hpp"

namespace MPP
{
	using Util::process;

	using Compiler = MIR::Toolchain::Compiler::Compiler;
	using MIR::Toolchain::Compiler::detect_compiler;
	using ELanguage = MIR::Toolchain::Language;
	using EMachine = MIR::Machines::Machine;

} // namespace MPP

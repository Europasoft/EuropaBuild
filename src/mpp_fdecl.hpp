#pragma once
//#include <cstdint>

// forward-declarations for types exposed by the MPP namespace wrapper in mpp.hpp
namespace MIR
{
    namespace Toolchain
    {
        namespace Compiler
        {
            class Compiler;
        }
        enum class Language : unsigned int;
    } // namespace Toolchain
    namespace Machines
    {
        enum class Machine : unsigned int;
    }
} // namespace MIR

// used to access the meson-plus-plus functions and types in a cleaner way
namespace MPP
{
    namespace Compilers = MIR::Toolchain::Compiler;
    using Compiler = MIR::Toolchain::Compiler::Compiler;
    using ELanguage = MIR::Toolchain::Language;
    using EMachine = MIR::Machines::Machine;
} // namespace MPP

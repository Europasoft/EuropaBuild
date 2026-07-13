#pragma once

#include <iostream>
#include <string>
#include <string_view>
#include <filesystem>

namespace EuropaBuild
{
	static inline constexpr auto SPLASH_MESSAGE = "EuropaBuild C++ /// Copyright Simon Liimatainen. All rights reserved.";
	static inline constexpr auto CONF_FILENAME = "EuropaBuild.json";
	static inline constexpr auto NINJA_FILENAME = "build.ninja";
	static inline constexpr auto INTERMEDIATE_DIR_NAME = "intermediate";
	inline std::filesystem::path GET_INTERMEDIATE_PATH()
	{
		return (std::filesystem::current_path() / std::filesystem::path(INTERMEDIATE_DIR_NAME));
	}

	static constexpr bool WinOS =
#if defined(_WIN32) || defined(_WIN64)
		true;
#else
		false;
#endif

	struct LogColors
	{
		static inline constexpr auto DEFAULT = "";
		static inline constexpr auto RESET = "\033[0m";
		static inline constexpr auto RED = "\033[31m";
		static inline constexpr auto BRIGHT_RED = "\033[91m";
		static inline constexpr auto GREEN = "\033[32m";
		static inline constexpr auto YELLOW = "\033[33m";
		static inline constexpr auto BLUE = "\033[34m";
		static inline constexpr auto MAGENTA = "\033[35m";
		static inline constexpr auto CYAN = "\033[36m";
		static inline constexpr auto WHITE = "\033[37m";
	};

	inline std::string colorMessage(std::string_view message, const char* color)
	{
		return (color + std::string(message) + LogColors::RESET);
	}

	inline void log(std::string_view message, const char* color = LogColors::RESET)
	{
		std::cout << colorMessage(message, color) << std::endl;
	}

	struct ConfigException : std::runtime_error
	{
		using std::runtime_error::runtime_error;
	};
	struct DependencyException : std::runtime_error
	{
		using std::runtime_error::runtime_error;
	};
	struct EnvironmentException : std::runtime_error
	{
		using std::runtime_error::runtime_error;
	};

} // namespace EuropaBuild

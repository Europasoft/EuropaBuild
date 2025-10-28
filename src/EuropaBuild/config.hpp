
#pragma once
#include <EuropaBuild/config_types.hpp>

namespace EuropaBuild
{
	enum class ESLogVerbosity : uint32_t
	{
		Verbose = 0,
		Warning = 1,
		Error = 2,
		Silent = 3
	};

	class ESLog
	{
	public:
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

		ESLogVerbosity verbosity = ESLogVerbosity::Verbose;

		static std::string inline colorMessage(std::string_view message, const char* color)
		{
			return (color + std::string(message) + ESLog::RESET);
		}

		void inline log(std::string_view message, ESLogVerbosity verbosity = ESLogVerbosity::Verbose, const char* color = RESET)
		{
			std::cout << colorMessage(message, color) << std::endl;
		}

		void inline warn(std::string_view message)
		{
			log(message, ESLogVerbosity::Warning, YELLOW);
		}

		void inline error(std::string_view message)
		{
			log(message, ESLogVerbosity::Error, BRIGHT_RED);
		}


	};
}

namespace EuropaBuild
{
	class ConfigUtils
	{
	public:
		static std::shared_ptr<BuildConfig2> parseConfigFromJson(const std::filesystem::path& fullPath);
		//static BuildConfig parseConfigFromCommands(int argc, char* argv[]);
		static void targetSanityCheck(const JSON::ObjectPtr& targetJson);
		static ETargetType targetTypeFromString(std::string_view str);
		static std::vector<fs::path> vectorStringsToPaths(std::vector<std::string> strs);
		static fs::path sanitizeStringToPath(std::string_view sv);
		static std::string lowercase(std::string_view sv);
	};

	
	
} // namespace EuropaBuild

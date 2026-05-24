// Logging implementation using spdlog

#include "util/logging.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/fmt/fmt.h>

static std::shared_ptr<spdlog::logger> GLogger;

static const char* CategoryToString(ELogCategory Cat)
{
    switch (Cat)
    {
        case ELogCategory::LogInit: return "Init";
        case ELogCategory::LogDev: return "Dev";
        case ELogCategory::LogNet: return "Net";
        case ELogCategory::LogGame: return "Game";
        case ELogCategory::LogHook: return "Event";
        case ELogCategory::LogLoot: return "Loot";
        case ELogCategory::LogPlayer: return "Player";
        case ELogCategory::LogReplication: return "Repl";
        case ELogCategory::LogMatchmaker: return "Match";
        case ELogCategory::LogConfig: return "Config";
        case ELogCategory::LogAsset: return "Asset";
        default: return "Unknown";
    }
}

namespace Logging
{

void Initialize(const std::string& LogFile)
{
    try
    {
        std::vector<spdlog::sink_ptr> Sinks;

        // Console sink with color
        auto ConsoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        ConsoleSink->set_pattern("[%H:%M:%S] [%^%l%$] %v");
        Sinks.push_back(ConsoleSink);

        // File sink (if path provided)
        if (!LogFile.empty())
        {
            auto FileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(LogFile, true);
            FileSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
            Sinks.push_back(FileSink);
        }

        GLogger = std::make_shared<spdlog::logger>("reboot", Sinks.begin(), Sinks.end());
        GLogger->set_level(spdlog::level::debug);
        GLogger->flush_on(spdlog::level::info);
        spdlog::set_default_logger(GLogger);
    }
    catch (const spdlog::spdlog_ex& Ex)
    {
        fmt::print(stderr, "Logger initialization failed: {}\n", Ex.what());
    }
}

void Shutdown()
{
    if (GLogger)
    {
        GLogger->flush();
    }
    spdlog::shutdown();
}

void Info(ELogCategory Cat, const std::string& Message)
{
    if (GLogger)
        GLogger->info("[{}] {}", CategoryToString(Cat), Message);
}

void Warn(ELogCategory Cat, const std::string& Message)
{
    if (GLogger)
        GLogger->warn("[{}] {}", CategoryToString(Cat), Message);
}

void Error(ELogCategory Cat, const std::string& Message)
{
    if (GLogger)
        GLogger->error("[{}] {}", CategoryToString(Cat), Message);
}

void Debug(ELogCategory Cat, const std::string& Message)
{
    if (GLogger)
        GLogger->debug("[{}] {}", CategoryToString(Cat), Message);
}

} // namespace Logging

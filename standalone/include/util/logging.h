#pragma once

// Logging - Replaces the original log.h with spdlog

#include <string>
#include <memory>
#include <spdlog/fmt/fmt.h>

// Log categories matching the original project
enum class ELogCategory
{
    LogInit,
    LogDev,
    LogNet,
    LogGame,
    LogHook,       // kept for compatibility, now means "event dispatch"
    LogLoot,
    LogPlayer,
    LogReplication,
    LogMatchmaker,
    LogConfig,
    LogAsset,
};

namespace Logging
{
    void Initialize(const std::string& LogFile = "");
    void Shutdown();

    void Info(ELogCategory Cat, const std::string& Message);
    void Warn(ELogCategory Cat, const std::string& Message);
    void Error(ELogCategory Cat, const std::string& Message);
    void Debug(ELogCategory Cat, const std::string& Message);
}

// Convenience macros
#define LOG_INFO(cat, ...) Logging::Info(ELogCategory::cat, fmt::format(__VA_ARGS__))
#define LOG_WARN(cat, ...) Logging::Warn(ELogCategory::cat, fmt::format(__VA_ARGS__))
#define LOG_ERROR(cat, ...) Logging::Error(ELogCategory::cat, fmt::format(__VA_ARGS__))
#define LOG_DEBUG(cat, ...) Logging::Debug(ELogCategory::cat, fmt::format(__VA_ARGS__))

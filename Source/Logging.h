#pragma once

#include "SRCommon.h"

SR_DISABLE_WARNINGS
#include <spdlog/spdlog.h>
SR_ENABLE_WARNINGS

#define SR_LOG(level, ...)    
#define SR_LOG_VERBOSE(...)    ::SR::gLogger->debug(__VA_ARGS__);
#define SR_LOG_INFO(...)       ::SR::gLogger->info(__VA_ARGS__);
#define SR_LOG_WARNING(...)    ::SR::gLogger->warn(__VA_ARGS__);
#define SR_LOG_ERROR(...)      ::SR::gLogger->error(__VA_ARGS__);
#define SR_LOG_FATAL(...)      ::SR::gLogger->critical(__VA_ARGS__);

enum class LogLevel
{
    Verbose,
    Info,
    Warning,
    Error,
    Fatal,
};

namespace SR
{
	extern std::shared_ptr<spdlog::logger> gLogger;
    extern void LogSystemInit();
}
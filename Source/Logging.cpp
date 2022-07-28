#include "Logging.h"

SR_DISABLE_WARNINGS
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
SR_ENABLE_WARNINGS

namespace SR
{
    std::shared_ptr<spdlog::logger> gLogger = nullptr;

    void LogSystemInit()
    {
        std::vector<spdlog::sink_ptr> sinks;
        sinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
        sinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("SR.log", true));
        sinks[0]->set_pattern("%^[%T] %n: %v%$");
        sinks[1]->set_pattern("[%T] [%l] %n: %v");
        auto color_sink = static_cast<spdlog::sinks::stdout_color_sink_mt*>(sinks[0].get());
        color_sink->set_color(spdlog::level::info, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        color_sink->set_color(spdlog::level::warn, FOREGROUND_RED | FOREGROUND_GREEN);
        color_sink->set_color(spdlog::level::err, FOREGROUND_RED);
        gLogger = std::make_shared<spdlog::logger>("Console Logger", begin(sinks), end(sinks));
        spdlog::register_logger(gLogger);
        gLogger->set_level(spdlog::level::debug);
        gLogger->flush_on(spdlog::level::debug);
    }
}
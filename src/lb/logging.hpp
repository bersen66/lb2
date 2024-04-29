#pragma once


#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include <spdlog/spdlog.h>
#include <lb/formatters.hpp>


#define TRACE(...) SPDLOG_LOGGER_TRACE(spdlog::get("multi-sink"), __VA_ARGS__)
#define DEBUG(...) SPDLOG_LOGGER_DEBUG(spdlog::get("multi-sink"), __VA_ARGS__)
#define INFO(...) SPDLOG_LOGGER_INFO(spdlog::get("multi-sink"), __VA_ARGS__)
#define WARN(...) SPDLOG_LOGGER_WARN(spdlog::get("multi-sink"), __VA_ARGS__)
#define ERROR(...) SPDLOG_LOGGER_ERROR(spdlog::get("multi-sink"), __VA_ARGS__)
#define CRITICAL(...) SPDLOG_LOGGER_CRITICAL(spdlog::get("multi-sink"), __VA_ARGS__)

// Embed stacktrace into error message
#define STR_WITH_STACKTRACE(...) fmt::format(__VA_ARGS__) + fmt::format("\nStacktrace:\n{}", boost::stacktrace::stacktrace())
#define STRACE(...) SPDLOG_LOGGER_TRACE(spdlog::get("multi-sink"), STR_WITH_STACKTRACE(__VA_ARGS__))
#define SDEBUG(...) SPDLOG_LOGGER_DEBUG(spdlog::get("multi-sink"), STR_WITH_STACKTRACE(__VA_ARGS__))
#define SINFO(...) SPDLOG_LOGGER_INFO(spdlog::get("multi-sink"),   STR_WITH_STACKTRACE(__VA_ARGS__))
#define SWARN(...) SPDLOG_LOGGER_WARN(spdlog::get("multi-sink"),   STR_WITH_STACKTRACE(__VA_ARGS__))
#define SERROR(...) SPDLOG_LOGGER_ERROR(spdlog::get("multi-sink"), STR_WITH_STACKTRACE(__VA_ARGS__))
#define SCRITICAL(...) SPDLOG_LOGGER_CRITICAL(spdlog::get("multi-sink"), STR_WITH_STACKTRACE(__VA_ARGS__))

#define EXCEPTION(...) throw std::runtime_error(fmt::format(__VA_ARGS__))
#define STACKTRACE(...) throw std::runtime_error(STR_WITH_STACKTRACE(__VA_ARGS__))
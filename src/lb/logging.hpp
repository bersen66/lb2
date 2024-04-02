#pragma once

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include <spdlog/spdlog.h>

#define TRACE(...) SPDLOG_LOGGER_TRACE(spdlog::get("multi-sink"), __VA_ARGS__)
#define DEBUG(...) SPDLOG_LOGGER_DEBUG(spdlog::get("multi-sink"), __VA_ARGS__)
#define INFO(...) SPDLOG_LOGGER_INFO(spdlog::get("multi-sink"), __VA_ARGS__)
#define WARN(...) SPDLOG_LOGGER_WARN(spdlog::get("multi-sink"), __VA_ARGS__)
#define ERROR(...) SPDLOG_LOGGER_ERROR(spdlog::get("multi-sink"), __VA_ARGS__)
#define CRITICAL(...) SPDLOG_LOGGER_CRITICAL(spdlog::get("multi-sink"), __VA_ARGS__)
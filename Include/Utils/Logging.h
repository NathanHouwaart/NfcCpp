/**
 * @file Logging.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief Logging utilities
 * @version 0.1
 * @date 2025-11-03
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <cstdarg>
#include <cstdio>

#define ENABLE_LOGGING 1

#define LOG_INFO(fmt, ...)  Logger::log("INFO", __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  Logger::log("WARN", __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) Logger::log("ERROR", __FILE__, __LINE__, fmt, ##__VA_ARGS__)

class Logger {
public:
    static void log(const char* level, const char* file, int line, const char* fmt, ...) {
#if ENABLE_LOGGING
        // Optional: prefix with level, file and line
        std::printf("[%s][%s:%d] ", level, file, line);

        va_list args;
        va_start(args, fmt);
        std::vprintf(fmt, args);  // or snprintf into a fixed buffer for embedded
        va_end(args);

        std::printf("\n");
#endif
    }
};

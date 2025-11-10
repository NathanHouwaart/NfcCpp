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
#include <cstring>

#define ENABLE_LOGGING 1

#define LOG_INFO(fmt, ...)  Logger::log("INFO", __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  Logger::log("WARN", __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) Logger::log("ERROR", __FILE__, __LINE__, fmt, ##__VA_ARGS__)

class Logger {
public:
    // ANSI color codes
    static constexpr const char* COLOR_RESET   = "\033[0m";
    static constexpr const char* COLOR_RED     = "\033[31m";
    static constexpr const char* COLOR_YELLOW  = "\033[33m";
    static constexpr const char* COLOR_GREEN   = "\033[32m";
    static constexpr const char* COLOR_CYAN    = "\033[36m";
    static constexpr const char* COLOR_GRAY    = "\033[90m";

    static void log(const char* level, const char* file, int line, const char* fmt, ...) {
#if ENABLE_LOGGING
        // Choose color based on log level
        const char* color = COLOR_RESET;
        if (std::strcmp(level, "ERROR") == 0) {
            color = COLOR_RED;
        } else if (std::strcmp(level, "WARN") == 0) {
            color = COLOR_YELLOW;
        } else if (std::strcmp(level, "INFO") == 0) {
            color = COLOR_GREEN;
        }

        // Print colored level with file and line info
        std::printf("%s[%s]%s %s[%s:%d]%s ", 
                    color, level, COLOR_RESET,
                    COLOR_GRAY, file, line, COLOR_RESET);

        va_list args;
        va_start(args, fmt);
        std::vprintf(fmt, args);  // or snprintf into a fixed buffer for embedded
        va_end(args);

        std::printf("\n");
#endif
    }
};

#ifndef LOGGING_H
#define LOGGING_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// Define clean code comptaitble aliases for syslog's constants
#define LOG_EMERGENCY 0 // System is unusable - should not be used by applications
#define LOG_ALERT 1     // Should be corrected immediately - Example: Loss of the primary ISP connection
#define LOG_CRITICAL 2  // Critical conditions - Example: A failure in the system's primary application
#define LOG_ERROR 3     // Error conditions - Example: An application has exceeded its file storage limit and attempts to write are failing
#define LOG_WARNING 4   // May indicate that an error will occur if action is not taken - Example: A non-root file system has only 2GB remaining
#define LOG_NOTICE 5    // Events that are unusual, but not error conditions
#define LOG_INFO 6      // Normal operational messages that require no action - Example: An application has started, paused or ended successfully
#define LOG_DEBUG 7     // Information useful to developers for debugging the application

// Label for each log level
#define LOG_LABEL_0 "EMERGENCY"
#define LOG_LABEL_1 "ALERT"
#define LOG_LABEL_2 "CRITICAL"
#define LOG_LABEL_3 "ERROR"
#define LOG_LABEL_4 "WARNING"
#define LOG_LABEL_5 "NOTICE"
#define LOG_LABEL_6 "INFO"
#define LOG_LABEL_7 "DEBUG"

// ANSI color codes to use for console logging
#define LOG_COLOR_RED 31
#define LOG_COLOR_ORANGE 33
#define LOG_COLOR_GREEN 32
#define LOG_COLOR_BLUE 34

// Color to use for each log level
#define LOG_COLOR_0 LOG_COLOR_RED
#define LOG_COLOR_1 LOG_COLOR_RED
#define LOG_COLOR_2 LOG_COLOR_RED
#define LOG_COLOR_3 LOG_COLOR_RED
#define LOG_COLOR_4 LOG_COLOR_ORANGE
#define LOG_COLOR_5 LOG_COLOR_GREEN
#define LOG_COLOR_6 LOG_COLOR_GREEN
#define LOG_COLOR_7 LOG_COLOR_BLUE

// Define constants for support logging types (bitmasks)
#define LOGGING_CONSOLE 1
#define LOGGING_SYSLOG 2

#define _logging_logToFile(filePointer, level, ...) logging_logToFile(filePointer, LOG_LABEL_##level, LOG_COLOR_##level, __FILE__, __LINE__, __func__, __VA_ARGS__)

// Log to all enabled outputs
#define log(level, ...)                             \
  do {                                              \
    if (level > LOGGING_LEVEL)                      \
      break;                                        \
    _logging_logToFile(stderr, level, __VA_ARGS__); \
  } while (0)

extern uint8_t LOGGING_LEVEL;

// A general function that logs to specified file, can be both a path and stderr
void logging_logToFile(FILE *filePointer, const char *label, int color, const char *file, int line, const char *function, const char *format, ...) __attribute__((nonnull(1)));

#endif

#ifndef SCOM_SHARED_LOG_H
#define SCOM_SHARED_LOG_H

#define DEFAULT_LOG_LEVEL DEBUG
#define DEFAULT_LOG_IDENTIFIER "default"
#define DEFAULT_LOG_FILE_PATH "../output.log"
#define LOG_TIME_FORMAT "%Y-%m-%d %H:%M:%S"
#define COLOR_RESET "\033[0m"

#include <stddef.h>
#include <stdbool.h>

#define LOG_LEVEL_LIST \
    X(DEBUG)      \
    X(INFO)       \
    X(WARN)       \
    X(ERROR)      \
    X(CRITICAL)

#define X(name) name,
typedef enum {
    LOG_LEVEL_LIST
    LOG_COUNT
} LogLevel;
#undef X

typedef struct {
    LogLevel loggerLevel;
    char *identifier;  // identifier to be used will display with the log messages
    char *log_file_path; // defaults to output.log if it isnt set
    bool is_regular_file;
    FILE *logFile;
} LoggerConfig;

// func definitions
// instead of using a complex backtrace() we can just pass the needed informaton using a macro at compile time
int _logfmt_internal(const char *filename, const int lineno, const char *func_name, FILE *fd, LogLevel level, const char *fmt, ...);

#define logfmt(fd, level, fmt, ...) \
    _logfmt_internal(__FILE__, __LINE__, __func__, fd, level, fmt, ##__VA_ARGS__) // super fascinating

int init_default_logger(LoggerConfig *config_override);
void cleanup_logger(void);
#endif
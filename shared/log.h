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
    const char *identifier;  // identifier to be used will display with the log messages
    const char *log_file_path; // defaults to output.log if it isnt set
    bool is_regular_file;
    FILE *logFile;
} LoggerConfig;

// func definitions
int logfmt(FILE *fd, LogLevel level, const char *fmt, ...);
int init_default_logger(LoggerConfig *config_override);
void cleanup_logger(void);
#endif
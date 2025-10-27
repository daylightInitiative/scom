#ifndef SCOM_CLIENT_LOG_H
#define SCOM_CLIENT_LOG_H

#define CURRENT_LOG_LEVEL DEBUG
#define LOG_FILE_PATH "../client.log"
#define LOG_TIME_FORMAT "%Y-%m-%d %H:%M:%S"
#define COLOR_RESET "\033[0m"

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

// func definitions
int logfmt(FILE *fd, LogLevel level, const char *fmt, ...);
#endif
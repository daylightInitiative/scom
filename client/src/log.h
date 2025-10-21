#ifndef SCOM_CLIENT_LOG_H
#define SCOM_CLIENT_LOG_H

#define COLOR_RESET "\033[0m"

#define LOG_LEVEL_LIST \
    X(INFO)       \
    X(DEBUG)      \
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
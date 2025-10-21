
#include <stdio.h>
#include <stdarg.h>

#include <sys/stat.h>
#include <unistd.h>

#include "log.h"

// this way its easy to add new log levels

const char *color_codes[] = {
    "\033[94;20m",
    "\033[92;20m",
    "\033[33;20m",
    "\033[31;20m",
    "\033[31;1m"
};

#define X(name) #name,
const char *level_names[] = {
    LOG_LEVEL_LIST
};
#undef X

const char *getLevelColor(LogLevel level) {
    if (level >= 0 && level < LOG_COUNT) {
        return color_codes[level];
    }
    return "\033[38;20m"; // default to gray
}

const char *getLogLevel(LogLevel level) {
    if (level >= 0 && level < LOG_COUNT) {
        return level_names[level];
    }
    return "INFO";
}

// its important to va_args to have the last named argument here fmt or whatever we use
int logfmt(FILE *fd, LogLevel level, const char *fmt, ...) {

    struct stat sb;
    int stream_fd = fileno(fd);
    if (fstat(stream_fd, &sb) < 0) {
        perror("fstat");
        return 1;
    }

    int is_regular = (sb.st_mode & S_IFMT) == S_IFREG;

    va_list args;
    va_start(args, fmt);
    
    const char *logLevel = getLogLevel(level);
    const char *logColor = getLevelColor(level);

    // this is a file not a stream, no colors
    if (is_regular) {
        fprintf(fd, "[%s] ", logLevel);
        vfprintf(fd, fmt, args);
    } else {
        fprintf(fd, "%s[%s] ", logColor, logLevel);
        vfprintf(fd, fmt, args);
        fprintf(fd, "%s\n", COLOR_RESET);
    }

    va_end(args);

    return 0;
}
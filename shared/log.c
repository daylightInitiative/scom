
#include <stdio.h>
#include <stdlib.h>

#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "log.h"

// this way its easy to add new log levels

static LoggerConfig *defaultLogger = NULL;



#define X(name) #name,
const char *level_names[] = {
    LOG_LEVEL_LIST
};
#undef X

const char *color_codes[] = {
    "\033[94;20m",
    "\033[92;20m",
    "\033[33;20m",
    "\033[31;20m",
    "\033[31;1m"
};

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

const char *get_timestamp(void) {

    struct timeval tv;
    gettimeofday(&tv, NULL);

    struct tm *tm_info = localtime(&tv.tv_sec);
    char time_buffer[64];
    memset(time_buffer, '\0', sizeof(time_buffer));
    strftime(time_buffer, sizeof(time_buffer), LOG_TIME_FORMAT, tm_info);

    static char timestamp[128]; // i'm pretty sure we can spare 8000 years
    memset(timestamp, '\0', sizeof(timestamp));
    snprintf(timestamp, sizeof(timestamp), "%s.%03ld", time_buffer, tv.tv_usec / 1000);

    return timestamp;
}

int init_default_logger(LoggerConfig *config_override) {

    if (config_override) {
        defaultLogger = config_override;
        return 0;
    }

    if (defaultLogger == NULL) {
        
        // lets create the file
        LoggerConfig *lc = malloc(sizeof(LoggerConfig));
        memset(lc, '\0', sizeof(LoggerConfig));

        lc->loggerLevel = DEFAULT_LOG_LEVEL;
        lc->logFile = fopen(DEFAULT_LOG_FILE_PATH, "w");
        if (!lc->logFile) {
            perror("fopen");
            return -1;
        }

        const char *loggingStarted = get_timestamp();
        fprintf(lc->logFile, "\n%s [INFO] Logging started.\n", loggingStarted);
        fflush(lc->logFile);

        defaultLogger = lc;
    }

    return 0;
}

void cleanup_logger() {
    if (defaultLogger != NULL) {
        if (defaultLogger->logFile) {
            fclose(defaultLogger->logFile);
            defaultLogger->logFile = NULL;
        }

        free(defaultLogger);
        defaultLogger = NULL;
    }
}


// its important to va_args to have the last named argument here fmt or whatever we use
int logfmt(FILE *fd, LogLevel level, const char *fmt, ...) {

    if (defaultLogger == NULL) {
        fprintf(stderr, "Logger is not initialized/allocated did you forget to call init_default_logger?\n");
        return 1;
    }

    // eventually we'll read from a json file or some kind of config file
    if (level < defaultLogger->loggerLevel) {
        return 0;
    }

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

    const char *timestamp = get_timestamp();

    // this is a file not a stream, no colors
    if (is_regular) {
        fprintf(fd, "%s - [%s] ", timestamp, logLevel);
        vfprintf(fd, fmt, args);
    } else {
        fprintf(fd, "%s%s - [%s] ", logColor, timestamp, logLevel);
        vfprintf(fd, fmt, args);
        fprintf(fd, "%s\n", COLOR_RESET);
    }

    va_end(args);

    return 0;
}
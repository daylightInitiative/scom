
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

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

bool is_regular_file_or_stream(FILE *fd) {
    struct stat sb;
    if (fstat(fileno(fd), &sb) < 0) return false;
    return (sb.st_mode & S_IFMT) == S_IFREG;
}

int valid_file_exists(const char *filepath) {

    if (access(filepath, W_OK | F_OK) != 0) {
        perror("access");
        return -1;
    }

    return 0;
}

int init_default_logger(LoggerConfig *config_override) {

    // we werent allocating new memory to be defaultLogger
    // but instead just assigning it to a stack struct pointer resulting in an invalid free across both client/server
    if (config_override) {
        defaultLogger = malloc(sizeof(LoggerConfig));
        if (!defaultLogger) {
            perror("malloc");
            return -1;
        }

        memset(defaultLogger, 0, sizeof(LoggerConfig));

        // we have to use strdup here because if we just assign it, it will use the old memory allocated string pointers
        defaultLogger->loggerLevel = config_override->loggerLevel;
        defaultLogger->identifier = config_override->identifier ? strdup(config_override->identifier) : NULL;
        defaultLogger->log_file_path = config_override->log_file_path ? strdup(config_override->log_file_path) : strdup(DEFAULT_LOG_FILE_PATH);

        assert(defaultLogger->identifier && "Missing LoggerConfig.identifier");
        assert(defaultLogger->log_file_path && "Missing LoggerConfig.log_file_path");

        defaultLogger->logFile = fopen(defaultLogger->log_file_path, "w");
        if (!defaultLogger->logFile) {
            perror("fopen");
            free(defaultLogger->identifier);
            free(defaultLogger->log_file_path);
            free(defaultLogger);
            defaultLogger = NULL;
            return -1;
        }

        defaultLogger->is_regular_file = is_regular_file_or_stream(defaultLogger->logFile);
        return 0;
    }

    // if there isnt a logger object already, we need to make one manually
    if (defaultLogger == NULL) {
        LoggerConfig *lc = malloc(sizeof(LoggerConfig));
        if (!lc) {
            perror("malloc");
            return -1;
        }

        memset(lc, 0, sizeof(LoggerConfig));
        lc->loggerLevel = DEFAULT_LOG_LEVEL;

        // always strdup so we dont have to worry about memory ownership issues K.I.S.S
        lc->log_file_path = strdup(DEFAULT_LOG_FILE_PATH);
        lc->identifier = strdup("DEFAULT");

        lc->logFile = fopen(DEFAULT_LOG_FILE_PATH, "w");
        if (!lc->logFile) {
            perror("fopen");
            free(lc->log_file_path);
            free(lc->identifier);
            free(lc);
            return -1;
        }

        lc->is_regular_file = true;

        const char *loggingStarted = get_timestamp();
        fprintf(lc->logFile, "\n%s [INFO] Logging started.\n", loggingStarted);

        // its important to flush here because C i/o stores prints in a buffer, but when an error occurs it should display immediately
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

        free(defaultLogger->identifier);
        free(defaultLogger->log_file_path);

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

    

    va_list args;
    va_start(args, fmt);
    
    const char *logLevel = getLogLevel(level);
    const char *logColor = getLevelColor(level);

    const char *timestamp = get_timestamp();

    char identifier[64];
    memset(identifier, '\0', sizeof(identifier));

    if (defaultLogger->identifier) {
        size_t len = strnlen(defaultLogger->identifier, sizeof(identifier) - 1);

        // its faster to use memcpy here since we dont need any formatting
        memcpy(identifier, defaultLogger->identifier, len);
        identifier[len] = '\0';
    } else {
        size_t len = strnlen(DEFAULT_LOG_IDENTIFIER, sizeof(identifier) - 1);
        memcpy(identifier, DEFAULT_LOG_IDENTIFIER, len);
        identifier[len] = '\0';
    }

    // this is a file not a stream, no colors
    // instead of checking the log files status, we need to check the fd regular file

    bool is_regular_file = is_regular_file_or_stream(fd);

    if (is_regular_file) {
        fprintf(fd, "%s - %s - [%s] ", timestamp, identifier, logLevel);
        vfprintf(fd, fmt, args);
        fprintf(fd, "\n");
        fflush(fd);

        if (defaultLogger->logFile && fd != defaultLogger->logFile) {
            va_list fileargs;
            va_copy(fileargs, args);
            fprintf(defaultLogger->logFile, "%s - %s - [%s] ", timestamp, identifier, logLevel);
            vfprintf(defaultLogger->logFile, fmt, fileargs);
            fprintf(defaultLogger->logFile, "\n");
            fflush(defaultLogger->logFile);
            va_end(fileargs);
        }
    } else {
        fprintf(fd, "%s%s - %s - [%s] ", logColor, timestamp, identifier, logLevel);
        vfprintf(fd, fmt, args);
        fprintf(fd, "%s\n", COLOR_RESET);
        fflush(fd);
    }

    fflush(fd);

    va_end(args);

    return 0;
}
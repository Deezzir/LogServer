#ifndef __LOGGER_H__
#define __LOGGER_H__

// Log level Enum - starting from 0
enum LOG_LEVEL { DEBUG,
                 WARNING,
                 ERROR,
                 CRITICAL };
// Log level str
inline char log_level_str[][16] = {"DEBUG", "WARNING", "ERROR", "CRITICAL"};

int InitializeLog();
void SetLogLevel(LOG_LEVEL level);
void Log(LOG_LEVEL level, const char *prog, const char *func, int line, const char *message);
void ExitLog();

#endif  // __LOGGER_H__
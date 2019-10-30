#include <pthread.h>
#include <stdarg.h>
#include <time.h>

#include "logging.h"

// Specifies which levels to output to log
uint8_t LOGGING_LEVEL = LOG_DEBUG;

pthread_mutex_t mutex;

void logging_logToFile(FILE *filePointer, const char *label, int color, const char *file, int line, const char *function, const char *format, ...) {
  // Current time
  time_t calendarNow = time(NULL);
  struct tm timeInfo;
  localtime_r(&calendarNow, &timeInfo);

  pthread_mutex_lock(&mutex);
  // tm_mon is in range 0-11. Need to add 1 to get real month
  // tm_year is years since 1900
  fprintf(filePointer, "\x1b[90m[%02d/%02d/%04d %02d:%02d:%02d %s]", timeInfo.tm_mday, timeInfo.tm_mon + 1, timeInfo.tm_year + 1900, timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec, tzname[1] == 0 ? tzname[0] : tzname[1]);

  fprintf(filePointer, "[\x1b[%dm%s\x1b[90m][%s@%d][%s]\n    └──\x1b[0m ", color, label, file, line, function);
  // Print the text and arguments that comes from the log(log_lvl, "%d %s %c", a, b, c)
  va_list arguments;
  va_start(arguments, format);
  vfprintf(filePointer, format, arguments);
  va_end(arguments);

  fprintf(filePointer, "\n");
  pthread_mutex_unlock(&mutex);
}

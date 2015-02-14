#include "log.h"

#include <time.h>
#include <stdlib.h>
#include <stdarg.h>

static FILE* log_out = NULL;
static log_level log_lvl = LOG_WARN;

void log_on_exit(void) {
	if (log_out)
		fclose(log_out);
}

bool log_setup(const char* path, log_level level) {
	if (!path)
		log_out = stderr;
	else if (!(log_out = fopen(path, "a")))
		return false;

	log_lvl = level;
	atexit(&log_on_exit);

	return true;
}

void log_commit_raw(log_level level, const char* file, size_t line,
                    const char* format, ...) {
	if (!log_out || level < log_lvl)
		return;

	flockfile(log_out);

	// Generate tag
	char tag;
	switch (level) {
		case LOG_INFO:
			tag = 'I';
			break;

		case LOG_WARN:
			tag = 'W';
			break;

		case LOG_ERROR:
			tag = 'E';
			break;

		default:
			tag = 'D';
			break;
	}

	// Print log info
	time_t tm = time(NULL);
	char timestamp[100];
	strftime(timestamp, 100, "%c", localtime(&tm));

	fprintf(log_out, "%c %s [%s:%lu]: ", tag, timestamp, file, line);

	// Print log message
	va_list args;
	va_start(args, format);
	vfprintf(log_out, format, args);
	va_end(args);

	fputc('\n', log_out);
	fflush(log_out);
	funlockfile(log_out);
}
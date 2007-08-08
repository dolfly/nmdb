
#include <stdio.h>		/* vsprintf() */
#include <stdarg.h>
#include <sys/types.h> 		/* open() */
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h> 		/* write() */
#include <string.h>		/* strcmp(), strerror() */
#include <errno.h>		/* errno */

#include "log.h"
#include "common.h"


/* Logging file descriptor, -1 if logging is disabled */
int logfd = -1;


int log_init(void)
{
	if (settings.logfname == NULL) {
		logfd = -1;
		return 1;
	}

	if (strcmp(settings.logfname, "-") == 0) {
		logfd = 0;
	} else {
		logfd = open(settings.logfname, O_WRONLY | O_APPEND | O_CREAT);
		if (logfd < 0)
			return 0;
	}

	return 1;
}

void wlog(const char *fmt, ...)
{
	int r, tr;
	va_list ap;
	char str[MAX_LOG_STR];
	char timestr[MAX_LOG_STR];
	time_t t;
	struct tm *tmp;

	if (logfd == -1)
		return;

	t = time(NULL);
	tmp = localtime(&t);
	tr = strftime(timestr, MAX_LOG_STR, "%F %H:%M:%S ", tmp);

	va_start(ap, fmt);
	r = vsnprintf(str, MAX_LOG_STR, fmt, ap);
	va_end(ap);

	write(logfd, timestr, tr);
	write(logfd, str, r);
}

void errlog(const char *s)
{
	wlog("%s: %s\n", s, strerror(errno));
}


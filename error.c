#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "error.h"

const char *shell_name;

void print_err(const char *format, ...) {
	fprintf(stderr, "%s: ", shell_name);
	va_list ap;
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
	fputc('\n', stderr);
}

void print_errno(const char *format, ...) {
	fprintf(stderr, "%s: ", shell_name);
	if (format) {
		va_list ap;
		va_start(ap, format);
		vfprintf(stderr, format, ap);
		va_end(ap);
		fputs(": ", stderr);
	}
	fprintf(stderr, "%s\n", strerror(errno));
}

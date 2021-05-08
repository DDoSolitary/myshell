#pragma once

extern const char *shell_name;

void print_err(const char *format, ...) __attribute__((format(printf, 1, 2)));
void print_errno(const char *format, ...) __attribute__((format(printf, 1, 2)));

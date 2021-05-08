#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cmd.h"
#include "error.h"
#include "parse.h"
#include "run.h"

char cmdline[1024];

int main(int argc, char **argv) {
	if (argc == 0) abort();
	shell_name = argv[0];
	bool interactive = isatty(STDIN_FILENO);
	int last_exit_status = 0;
	while (true) {
		if (interactive) {
			printf("$ ");
		}
		fflush(stdout);
		if (!fgets(cmdline, sizeof(cmdline), stdin)) {
			if (ferror(stdin)) {
				print_err("could not read command");
				return 1;
			}
			break;
		}
		size_t cmd_len = strlen(cmdline);
		if (cmdline[cmd_len - 1] == '\n') {
			cmdline[cmd_len - 1] = '\0';
		}
		token_list_t tokens;
		token_list_init(&tokens);
		if (!tokenize(cmdline, &tokens)) {
			print_err("command syntax error");
			token_list_free(&tokens);
			if (interactive) {
				continue;
			} else {
				return 1;
			}
		}
		pipeline_t pipeline;
		pipeline_init(&pipeline);
		if (!parse(cmdline, &tokens, &pipeline)) {
			print_err("command syntax error");
			token_list_free(&tokens);
			pipeline_free(&pipeline);
			if (interactive) {
				continue;
			} else {
				return 1;
			}
		}
		token_list_free(&tokens);
		last_exit_status = run_pipeline(&pipeline);
		pipeline_free(&pipeline);
	}
	return last_exit_status;
}

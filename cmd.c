#include <stdlib.h>
#include <unistd.h>
#include "cmd.h"

void command_init(command_t *cmd) {
	vec_init(&cmd->args, sizeof(char *));
	vec_init(&cmd->redirs, sizeof(redir_t));
	// Add standard fds by default.
	redir_t redir;
	redir.from_path = NULL;
	redir.type = REDIR_FD_IN;
	redir.to_fd = STDIN_FILENO;
	redir.from_fd = STDIN_FILENO;
	command_redir_push_back(cmd, &redir);
	redir.type = REDIR_FD_OUT;
	redir.to_fd = STDOUT_FILENO;
	redir.from_fd = STDOUT_FILENO;
	command_redir_push_back(cmd, &redir);
	redir.to_fd = STDERR_FILENO;
	redir.from_fd = STDERR_FILENO;
	command_redir_push_back(cmd, &redir);
}

void command_free(command_t *cmd) {
	for (size_t i = 0; i < command_arg_size(cmd); i++) {
		free(*command_arg_at(cmd, i));
	}
	vec_free(&cmd->args);
	for (size_t i = 0; i < command_redir_size(cmd); i++) {
		redir_t *redir = command_redir_at(cmd, i);
		free(redir->from_path);
	}
	vec_free(&cmd->redirs);
}

void pipeline_free(pipeline_t *pipeline) {
	for (size_t i = 0; i < pipeline_size(pipeline); i++) {
		command_free(pipeline_at(pipeline, i));
	}
	vec_free(pipeline);
}

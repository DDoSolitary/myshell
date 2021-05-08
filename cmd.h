#pragma once

#include <stdbool.h>
#include "vector.h"

typedef enum {
	REDIR_FILE_IN,
	REDIR_FILE_TRUNC,
	REDIR_FILE_APPEND,
	REDIR_FILE_IN_OUT,
	REDIR_FD_IN,
	REDIR_FD_OUT,
	REDIR_FILE_MAX = REDIR_FILE_IN_OUT
} redir_type_t;

#define REDIR_IS_FILE(type) ((type) <= REDIR_FILE_MAX)
#define REDIR_IS_FD(type) ((type) > REDIR_FILE_MAX)

#define REDIR_IS_READABLE(type) ( \
	(type) == REDIR_FILE_IN || \
	(type) == REDIR_FILE_IN_OUT || \
	(type) == REDIR_FD_IN)

#define REDIR_IS_WRITABLE(type) ( \
	(type) == REDIR_FILE_TRUNC || \
	(type) == REDIR_FILE_APPEND || \
	(type) == REDIR_FILE_IN_OUT || \
	(type) == REDIR_FD_OUT)

typedef struct {
	redir_type_t type;
	int to_fd;
	int from_fd;
	char *from_path;
} redir_t;

typedef struct {
	vec_t args;
	vec_t redirs;
} command_t;

typedef vec_t pipeline_t;

static inline void pipeline_init(pipeline_t *pipeline) {
	vec_init(pipeline, sizeof(command_t));
}

static inline command_t *pipeline_at(pipeline_t *pipeline, size_t i) {
	return vec_at(pipeline, i);
}

static inline size_t pipeline_size(pipeline_t *pipeline) {
	return vec_size(pipeline);
}

static inline void pipeline_push_back(pipeline_t *pipeline, command_t *cmd) {
	vec_push_back(pipeline, cmd);
}

static inline char **command_arg_at(command_t *cmd, size_t i) {
	return vec_at(&cmd->args, i);
}

static inline size_t command_arg_size(command_t *cmd) {
	return vec_size(&cmd->args);
}

static inline void command_arg_push_back(command_t *cmd, char *arg) {
	vec_push_back(&cmd->args, &arg);
}

static inline void command_arg_pop_back(command_t *cmd) {
	vec_pop_back(&cmd->args);
}

static inline redir_t *command_redir_at(command_t *cmd, size_t i) {
	return vec_at(&cmd->redirs, i);
}

static inline size_t command_redir_size(command_t *cmd) {
	return vec_size(&cmd->redirs);
}

static inline void command_redir_push_back(command_t *cmd, redir_t *redir) {
	vec_push_back(&cmd->redirs, redir);
}

void command_init(command_t *cmd);
void command_free(command_t *cmd);
void pipeline_free(pipeline_t *pipeline);

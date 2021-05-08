#pragma once

#include <stdbool.h>
#include <stddef.h>
#include "cmd.h"
#include "vector.h"

typedef enum {
	TOKEN_WORD,
	TOKEN_OP,
	TOKEN_END
} token_type_t;

typedef struct {
	size_t off;
	size_t len;
	token_type_t type;
} token_info_t;


typedef vec_t token_list_t;


static inline void token_list_init(token_list_t *tokens) {
	vec_init(tokens, sizeof(token_info_t));
}

static inline void token_list_free(token_list_t *tokens) {
	vec_free(tokens);
}

bool tokenize(const char *input, token_list_t *tokens);
bool parse(const char *input, token_list_t *tokens, pipeline_t *pipeline);

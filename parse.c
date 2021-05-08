#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "parse.h"

typedef enum {
	QUOTE_NONE,
	QUOTE_SINGLE,
	QUOTE_DOUBLE,
	QUOTE_BACKSLASH,
	QUOTE_BACKSLASH_IN_DOUBLE
} quote_state_t;

static bool check_quote(quote_state_t *state, char c) {
	bool quoted = false;
	switch (*state) {
	case QUOTE_SINGLE:
		if (c == '\'') {
			*state = QUOTE_NONE;
		} else {
			quoted = true;
		}
		break;
	case QUOTE_DOUBLE:
		if (c == '\\') {
			*state = QUOTE_BACKSLASH_IN_DOUBLE;
		} else if (c == '"') {
			*state = QUOTE_NONE;
		} else {
			quoted = true;
		}
		break;
	case QUOTE_BACKSLASH:
		*state = QUOTE_NONE;
		quoted = true;
		break;
	case QUOTE_BACKSLASH_IN_DOUBLE:
		*state = QUOTE_DOUBLE;
		quoted = true;
		break;
	default: // QUOTE_NONE
		if (c == '\'') {
			*state = QUOTE_SINGLE;
		} else if (c == '"') {
			*state = QUOTE_DOUBLE;
		} else if (c == '\\') {
			*state = QUOTE_BACKSLASH;
		}
		break;
	}
	return quoted;
}

static void end_token(token_info_t *token, token_list_t *tokens) {
	if (token->len) {
		vec_push_back(tokens, token);
	}
	token->off += token->len;
	token->len = 0;
	token->type = TOKEN_WORD;
}

static bool is_quote_char(char c) {
	return c == '\'' || c == '"' || c == '\\';
}

bool tokenize(const char *input, token_list_t *tokens) {
	quote_state_t quote_state = QUOTE_NONE;
	token_info_t cur_token = { .off = 0, .len = 0, .type = TOKEN_WORD };
	for (size_t pos = 0;; pos++) {
		char c = input[pos];
		bool quoted = check_quote(&quote_state, c);
		if (!c) {
			if (quoted) return false;
			end_token(&cur_token, tokens);
			cur_token.type = TOKEN_END;
			vec_push_back(tokens, &cur_token);
			break;
		}
		if (cur_token.type == TOKEN_OP) {
			// @formatter:off
			if (!quoted && cur_token.len == 1 && (
				(input[cur_token.off] == '<' && (c == '<' || c == '>' || c == '&')) ||
				(input[cur_token.off] == '>' && (c == '>' || c == '|' || c == '&')))) {
			// @formatter:on
				cur_token.len++;
				continue;
			} else {
				end_token(&cur_token, tokens);
			}
		}
		if (!quoted) {
			if (is_quote_char(c)) {
				cur_token.len++;
				continue;
			} else if (c == '<' || c == '>' || c == '|') {
				end_token(&cur_token, tokens);
				cur_token.len = 1;
				cur_token.type = TOKEN_OP;
				continue;
			} else if (isblank(c)) {
				end_token(&cur_token, tokens);
				cur_token.off++;
				continue;
			}
		}
		cur_token.len++;
	}
	return true;
}

static char *unquote(const char *input, token_info_t *token) {
	char *output = malloc(token->len + 1);
	quote_state_t state = QUOTE_NONE;
	size_t out_pos = 0;
	for (size_t i = 0; i < token->len; i++) {
		char c = input[token->off + i];
		if (state == QUOTE_BACKSLASH_IN_DOUBLE && c != '\\' && c != '"') {
			output[out_pos++] = '\\';
		}
		bool quoted = check_quote(&state, c);
		if (!is_quote_char(c) || quoted) {
			output[out_pos++] = c;
		}
	}
	output[out_pos] = '\0';
	return output;
}

static bool token_is_op(const char *input, token_info_t *token, const char *op) {
	if (token->type != TOKEN_OP || token->len != strlen(op)) {
		return false;
	}
	return !strncmp(op, input + token->off, token->len);
}

static bool parse_command(const char *input, token_info_t *tokens, size_t len, command_t *cmd) {
	if (!len) return false;
	redir_t redir;
	for (size_t i = 0; i < len; i++) {
		token_info_t *token = &tokens[i];
		if (token->type == TOKEN_WORD) {
			char *word = unquote(input, token);
			vec_push_back(&cmd->args, &word);
		} else { // TOKEN_OP
			if (i + 1 == len) return false;
			token_info_t *next_token = &tokens[i + 1];
			if (next_token->type != TOKEN_WORD) return false;
			if (token_is_op(input, token, "<")) {
				redir.type = REDIR_FILE_IN;
				redir.to_fd = 0;
			} else if (token_is_op(input, token, ">") || token_is_op(input, token, ">|")) {
				redir.type = REDIR_FILE_TRUNC;
				redir.to_fd = 1;
			} else if (token_is_op(input, token, ">>")) {
				redir.type = REDIR_FILE_APPEND;
				redir.to_fd = 1;
			} else if (token_is_op(input, token, "<>")) {
				redir.type = REDIR_FILE_IN_OUT;
				redir.to_fd = 0;
			} else if (token_is_op(input, token, "<&")) {
				redir.type = REDIR_FD_IN;
				redir.to_fd = 0;
			} else if (token_is_op(input, token, ">&")) {
				redir.type = REDIR_FD_OUT;
				redir.to_fd = 1;
			} else {
				return false;
			}
			redir.from_path = unquote(input, next_token);
			if (i > 0) {
				token_info_t *prev_token = &tokens[i - 1];
				if (prev_token->type == TOKEN_WORD && prev_token->off + prev_token->len == token->off) {
					char *end;
					long fd = strtol(input + prev_token->off, &end, 10);
					if (end == input + token->off && fd >= 0 && fd <= INT_MAX) {
						redir.to_fd = (int)fd;
						free(*command_arg_at(cmd, command_arg_size(cmd) - 1));
						vec_pop_back(&cmd->args);
					}
				}
			}
			redir.from_fd = -1;
			vec_push_back(&cmd->redirs, &redir);
			i++;
		}
	}
	// Make execvp happy.
	char *null = NULL;
	vec_push_back(&cmd->args, &null);
	return true;
}

bool parse(const char *input, token_list_t *tokens, pipeline_t *pipeline) {
	size_t last = 0;
	command_t cmd;
	for (size_t i = 0; i < vec_size(tokens); i++) {
		token_info_t *token = vec_at(tokens, i);
		if (token->type == TOKEN_END || token_is_op(input, token, "|")) {
			command_init(&cmd);
			if (parse_command(input, vec_at(tokens, last), i - last, &cmd)) {
				vec_push_back(pipeline, &cmd);
				last = i + 1;
			} else {
				command_free(&cmd);
				return false;
			}
		}
	}
	return true;
}

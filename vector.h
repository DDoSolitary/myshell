#pragma once

#include <stddef.h>

typedef struct {
	size_t size;
	size_t capacity;
	size_t ele_size;
	void *data;
} vec_t;

void vec_init(vec_t *v, size_t ele_size);
void vec_free(vec_t *v);
void *vec_at(vec_t *v, size_t i);
void vec_push_back(vec_t *v, void *data);
void vec_pop_back(vec_t *v);

static inline size_t vec_size(vec_t *v) {
	return v->size;
}

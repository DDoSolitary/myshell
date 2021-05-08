#include <stdlib.h>
#include <string.h>
#include "vector.h"

void vec_init(vec_t *v, size_t ele_size) {
	v->size = 0;
	v->capacity = 0;
	v->ele_size = ele_size;
	v->data = NULL;
}

void vec_free(vec_t *v) {
	if (v->data) {
		free(v->data);
	}
}

void *vec_at(vec_t *v, size_t i) {
	return (char *)v->data + v->ele_size * i;
}

void vec_push_back(vec_t *v, void *data) {
	if (v->size == v->capacity) {
		size_t new_cap = v->capacity ? v->capacity * 2 : 1;
		void *new_data = malloc(new_cap * v->ele_size);
		if (!new_data) abort();
		memcpy(new_data, v->data, v->capacity * v->ele_size);
		free(v->data);
		v->capacity = new_cap;
		v->data = new_data;
	}
	if (data) {
		memcpy(vec_at(v, v->size), data, v->ele_size);
	}
	v->size++;
}

void vec_pop_back(vec_t *v) {
	v->size--;
}

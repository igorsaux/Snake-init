#pragma once

#include <stdint.h>

typedef struct {
    void*  _data;
    size_t _size;
    size_t _capacity;
    size_t _elem_size;
} SNK_Vec;

SNK_Vec SNK_Vec_new(size_t capacity, size_t elem_size, bool adjust_size);

void* SNK_Vec_data(const SNK_Vec* vec);

size_t SNK_Vec_size(const SNK_Vec* vec);

size_t SNK_Vec_capacity(const SNK_Vec* vec);

void SNK_Vec_push(SNK_Vec* vec, const void* elem, size_t elem_size);

void SNK_Vec_remove(SNK_Vec* vec, size_t index, void* out_elem);

void* SNK_Vec_at(const SNK_Vec* vec, size_t index);

void SNK_Vec_free(SNK_Vec* vec);

#include "vec.h"
#include "utils.h"
#include <stdio.h>

SNK_Vec SNK_Vec_new(const size_t capacity, const size_t elem_size, const bool adjust_size) {
    if (capacity == 0) {
        return (SNK_Vec){};
    }

    void* data = malloc(capacity * elem_size);

    if (data == nullptr)
        SNK_crash("Failed to allocate memory for vector");

    return (SNK_Vec){
        ._data      = data,
        ._size      = adjust_size ? capacity : 0,
        ._capacity  = capacity,
        ._elem_size = elem_size,
    };
}

void* SNK_Vec_data(const SNK_Vec* vec) {
    ASSERT(vec != nullptr);

    return vec->_data;
}

size_t SNK_Vec_size(const SNK_Vec* vec) {
    ASSERT(vec != nullptr);

    return vec->_size;
}

size_t SNK_Vec_capacity(const SNK_Vec* vec) {
    ASSERT(vec != nullptr);

    return vec->_capacity;
}

void SNK_Vec_push(SNK_Vec* vec, const void* elem, const size_t elem_size) {
    ASSERT(vec != nullptr);
    ASSERT(elem != nullptr);
    ASSERT(elem_size == vec->_elem_size);

    if (vec->_size == vec->_capacity) {
        vec->_capacity *= 2;
        vec->_data = realloc(vec->_data, vec->_capacity * vec->_elem_size);

        if (vec->_data == nullptr)
            SNK_crash("Failed to reallocate memory for vector");
    }

    memcpy((char*)vec->_data + vec->_size * vec->_elem_size, elem, elem_size);
    vec->_size++;
}

void SNK_Vec_remove(SNK_Vec* vec, const size_t index, void* out_elem) {
    ASSERT(vec != nullptr);
    ASSERT(index < vec->_size);

    if (out_elem != nullptr) {
        memcpy(out_elem, (char*)vec->_data + index * vec->_elem_size, vec->_elem_size);
    }

    if (index != vec->_size - 1) {
        memmove((char*)vec->_data + index * vec->_elem_size, (char*)vec->_data + (index + 1) * vec->_elem_size,
                (vec->_size - index - 1) * vec->_elem_size);
    }

    vec->_size--;
}

void* SNK_Vec_at(const SNK_Vec* vec, const size_t index) {
    ASSERT(vec != nullptr);

    if (index > vec->_size)
        return nullptr;

    return (char*)vec->_data + index * vec->_elem_size;
}

void SNK_Vec_free(SNK_Vec* vec) {
    ASSERT(vec != nullptr);

    if (vec->_data != nullptr) {
        free(vec->_data);
    }

    vec->_data      = nullptr;
    vec->_capacity  = 0;
    vec->_elem_size = 0;
    vec->_size      = 0;
}

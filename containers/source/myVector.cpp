#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

#include "utils.h"
#include "myVector.h"

const size_t MINIMUM_RESERVED = 5;

void *vectorGet(Vector_t *vec, size_t i) {
    assert(vec);
    assert(i < vec->size);
    return (char*)vec->base + i * vec->elemSize;
}

void *vectorFind(Vector_t *vec, const void *elem) {
    for (char *idx = vec->base; idx <= (char *)vectorGet(vec->base, vec->size - 1); idx += vec->elemSize) {
        if (memcpy(idx, elem)==0)
            return idx;
    }
    return NULL;
}

Vector_t vectorCopyCtor(size_t size, size_t elemSize, const void* data) {
    Vector_t newVector = vectorCtor(size, elemSize);
    memcpy(newVector.base, data, size*elemSize);
    return newVector;
}

void vectorDtor(Vector_t* vec) {
    assert(vec);
    free(vec->base);
    vec->base = NULL;
}

Vector_t vectorCtor(size_t size, size_t elemSize) {
    size_t reserved = (size < MINIMUM_RESERVED) ? MINIMUM_RESERVED : size;
    Vector_t newVector = {NULL, size, reserved, elemSize};
    newVector.base = calloc(reserved, elemSize);
    return newVector;
}

void* vectorPush(Vector_t* vec, const void* elem) {
    if (vec->size >= vec->reserved) {
        void *newBase = recalloc(vec->base, 2*vec->reserved*vec->elemSize, vec->reserved*vec->elemSize);
        if (!newBase) return NULL;
        vec->reserved *= 2;
        vec->base = newBase;
    }
    vec->size++;
    memcpy(vectorGet(*vec, vec->size-1), elem, vec->elemSize);
    return vectorGet(*vec, vec->size-1);
}

void *recalloc(void *base, size_t newSize, size_t oldSize) {
    void* newBase = realloc(base, newSize);
    if (!newBase) return NULL;
    if (newSize > oldSize)
        memset((char*)newBase + oldSize, 0, newSize-oldSize);
    return newBase;
}

void* vectorPop(Vector_t* vec) {
    if (vec->size == 0) return NULL;
    vec->size--;
    return vectorGet(*vec, vec->size-1);
}


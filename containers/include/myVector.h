#ifndef MY_VECTOR_H
#define MY_VECTOR_H

typedef struct Vector {
    void *base;
    size_t size;
    size_t reserved;
    size_t elemSize;
} Vector_t;

Vector_t vectorCtor(size_t size, size_t elemSize);

Vector_t vectorCopyCtor(size_t size, size_t elemSize, const void* data);

void vectorDtor(Vector_t* vec);

/*!
    @brief Pushes elem to end of the vector
    @return Pointer to pushed element in case of success, NULL otherwise
*/
void *vectorPush(Vector_t* vec, const void* elem);

/*!
    @brief Deletes last element in vector
    @return Pointer to new last element in case of success, NULL otherwise
*/
void *vectorPop(Vector_t *vec);

void *vectorGet(Vector_t *vec, size_t i);

#define vectorGetT(type, vec, i) ((type) vectorGet(vec, i))

void *vectorFind(Vector_t *vec, const void *elem);

void *recalloc(void *base, size_t newSize, size_t oldSize);
#endif

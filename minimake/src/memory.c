#include "memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *allocate(size_t size)
{
    void *pointer = malloc(size ? size : 1);

    if (!pointer)
    {
        perror("minimake: malloc");
        exit(2);
    }
    return pointer;
}

void *resize_allocation(void *pointer, size_t size)
{
    void *resized = realloc(pointer, size ? size : 1);

    if (!resized)
    {
        perror("minimake: realloc");
        exit(2);
    }
    return resized;
}

char *copy_slice(const char *text, size_t size)
{
    char *copy = allocate(size + 1);

    memcpy(copy, text, size);
    copy[size] = '\0';
    return copy;
}

char *copy_string(const char *text)
{
    return copy_slice(text, strlen(text));
}

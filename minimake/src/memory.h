#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>

void *allocate(size_t size);
void *resize_allocation(void *pointer, size_t size);
char *copy_string(const char *text);
char *copy_slice(const char *text, size_t size);

#endif /* ! MEMORY_H */

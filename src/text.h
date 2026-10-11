#ifndef TEXT_H
#define TEXT_H

#include <stdbool.h>
#include <stddef.h>

struct buffer
{
    char *data;
    size_t length;
    size_t capacity;
};

void append_buffer(struct buffer *buffer, const char *text);
void append_character(struct buffer *buffer, char character);
char *take_buffer(struct buffer *buffer);
char *skip_blanks(char *text);
char *trim_name(char *text);
bool valid_name(const char *name);

#endif /* ! TEXT_H */

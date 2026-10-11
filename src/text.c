#include "text.h"

#include <ctype.h>
#include <string.h>

#include "memory.h"

void append_buffer(struct buffer *buffer, const char *text)
{
    size_t length = strlen(text);
    size_t required = buffer->length + length + 1;

    if (required > buffer->capacity)
    {
        size_t capacity = buffer->capacity ? buffer->capacity : 32;

        while (capacity < required)
            capacity *= 2;
        buffer->data = resize_allocation(buffer->data, capacity);
        buffer->capacity = capacity;
    }
    memcpy(buffer->data + buffer->length, text, length + 1);
    buffer->length += length;
}

void append_character(struct buffer *buffer, char character)
{
    char text[2] = { character, '\0' };

    append_buffer(buffer, text);
}

char *take_buffer(struct buffer *buffer)
{
    char *result = buffer->data ? buffer->data : copy_string("");

    buffer->data = NULL;
    buffer->length = 0;
    buffer->capacity = 0;
    return result;
}

char *skip_blanks(char *text)
{
    while (*text == ' ' || *text == '\t')
        text++;
    return text;
}

char *trim_name(char *text)
{
    char *start = skip_blanks(text);
    size_t length = strlen(start);

    while (length && (start[length - 1] == ' ' || start[length - 1] == '\t'))
        start[--length] = '\0';
    return start;
}

bool valid_name(const char *name)
{
    const void *bytes = name;
    const unsigned char *cursor = bytes;

    if (!*name)
        return false;
    for (; *cursor; cursor++)
    {
        if (isblank(*cursor) || *cursor == ':' || *cursor == '#'
            || *cursor == '=')
            return false;
    }
    return true;
}

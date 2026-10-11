#include <stdlib.h>

#include "memory.h"
#include "model.h"
#include "text.h"

void append_string(struct string_list **list, const char *value)
{
    struct string_list *node = allocate(sizeof(*node));

    node->value = copy_string(value);
    node->next = NULL;
    while (*list)
        list = &(*list)->next;
    *list = node;
}

void destroy_strings(struct string_list *list)
{
    while (list)
    {
        struct string_list *next = list->next;

        free(list->value);
        free(list);
        list = next;
    }
}

char *join_strings(const struct string_list *list)
{
    struct buffer buffer = { 0 };

    for (; list; list = list->next)
    {
        if (buffer.length)
            append_character(&buffer, ' ');
        append_buffer(&buffer, list->value);
    }
    return take_buffer(&buffer);
}

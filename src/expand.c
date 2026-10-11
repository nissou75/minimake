#include "expand.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "text.h"

struct expansion_stack
{
    const char *name;
    const struct expansion_stack *previous;
};

struct expansion
{
    const struct makefile *makefile;
    const struct rule *rule;
    const struct expansion_stack *stack;
};

static char *expand_text(const struct expansion *context, const char *input);

static bool already_listed(const struct string_list *list,
                           const struct string_list *dependency)
{
    for (; list != dependency; list = list->next)
    {
        if (strcmp(list->value, dependency->value) == 0)
            return true;
    }
    return false;
}

static char *expand_dependencies(const struct rule *rule)
{
    struct buffer output = { 0 };
    const struct string_list *dependency = rule->dependencies;
    bool first = true;

    for (; dependency; dependency = dependency->next)
    {
        if (already_listed(rule->dependencies, dependency))
            continue;
        if (!first)
            append_character(&output, ' ');
        append_buffer(&output, dependency->value);
        first = false;
    }
    return take_buffer(&output);
}

static char *expand_automatic(const struct rule *rule, char name)
{
    if (!rule)
        return copy_string("");
    if (name == '@')
        return copy_string(rule->target);
    if (name == '<')
    {
        if (!rule->dependencies)
            return copy_string("");
        return copy_string(rule->dependencies->value);
    }
    return expand_dependencies(rule);
}

static bool has_recursive_name(const struct expansion *context,
                               const char *name)
{
    const struct expansion_stack *stack = context->stack;

    for (; stack; stack = stack->previous)
    {
        if (strcmp(stack->name, name) == 0)
            return true;
    }
    return false;
}

static char *expand_name(const struct expansion *context, const char *name)
{
    if (name[0] && !name[1] && strchr("@<^", name[0]))
        return expand_automatic(context->rule, name[0]);
    const char *value = find_variable(context->makefile, name);
    if (!value)
        value = getenv(name);
    if (!value)
        return copy_string("");
    if (has_recursive_name(context, name))
    {
        fprintf(stderr, "minimake: *** Recursive variable '%s'. Stop.\n", name);
        return NULL;
    }
    struct expansion_stack stack = { name, context->stack };
    struct expansion nested = *context;
    nested.stack = &stack;
    return expand_text(&nested, value);
}

static const char *find_reference_end(const char *input)
{
    char opening = *input;
    char closing = opening == '(' ? ')' : '}';
    const char *end = strchr(input + 1, closing);
    if (!end)
    {
        fprintf(stderr,
                "minimake: *** Unterminated variable reference. Stop.\n");
        return NULL;
    }
    size_t length = end - input - 1;
    if (!memchr(input + 1, '$', length))
        return end;
    size_t depth = 1;
    for (++input; *input; ++input)
    {
        if (*input == opening)
            ++depth;
        else if (*input == closing && --depth == 0)
            return input;
    }
    fprintf(stderr, "minimake: *** Unterminated variable reference. Stop.\n");
    return NULL;
}

static char *expand_reference(const struct expansion *context,
                              const char **position)
{
    const char *input = *position;
    const char *end = find_reference_end(input);
    if (!end)
        return NULL;
    size_t length = end - input - 1;
    char *raw_name = copy_slice(input + 1, length);
    char *name = expand_text(context, raw_name);
    free(raw_name);
    if (!name)
        return NULL;
    char *value = expand_name(context, name);
    free(name);
    *position = end + 1;
    return value;
}

static char *expand_dollar(const struct expansion *context,
                           const char **position)
{
    ++*position;
    if (**position == '(' || **position == '{')
        return expand_reference(context, position);
    if (**position == '$')
    {
        ++*position;
        return copy_string("$");
    }
    if (!**position)
        return copy_string("$");
    char name[2] = { **position, '\0' };
    ++*position;
    return expand_name(context, name);
}

static char *expand_text(const struct expansion *context, const char *input)
{
    struct buffer output = { 0 };

    while (*input)
    {
        if (*input != '$')
        {
            append_character(&output, *input);
            ++input;
            continue;
        }
        char *value = expand_dollar(context, &input);
        if (!value)
        {
            free(output.data);
            return NULL;
        }
        append_buffer(&output, value);
        free(value);
    }
    return take_buffer(&output);
}

char *expand_string(const struct makefile *makefile, const char *input,
                    const struct rule *rule)
{
    struct expansion context = { makefile, rule, NULL };
    return expand_text(&context, input);
}

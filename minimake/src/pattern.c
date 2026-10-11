#include "pattern.h"

#include <stdlib.h>
#include <string.h>

#include "memory.h"

bool match_pattern(const char *pattern, const char *target, size_t *stem_size)
{
    const char *wildcard = strchr(pattern, '%');
    if (!wildcard)
        return false;
    size_t prefix_size = wildcard - pattern;
    size_t suffix_size = strlen(wildcard + 1);
    size_t target_size = strlen(target);
    if (target_size < prefix_size + suffix_size)
        return false;
    if (strncmp(pattern, target, prefix_size) != 0
        || strcmp(wildcard + 1, target + target_size - suffix_size) != 0)
        return false;
    *stem_size = target_size - prefix_size - suffix_size;
    return true;
}

char *substitute_pattern(const char *pattern, const char *stem)
{
    const char *wildcard = strchr(pattern, '%');
    if (!wildcard)
        return copy_string(pattern);
    size_t prefix_size = wildcard - pattern;
    size_t stem_size = strlen(stem);
    size_t size = prefix_size + stem_size + strlen(wildcard + 1) + 1;
    char *result = allocate(size);
    memcpy(result, pattern, prefix_size);
    memcpy(result + prefix_size, stem, stem_size);
    strcpy(result + prefix_size + stem_size, wildcard + 1);
    return result;
}

static struct rule *instantiate_pattern(struct makefile *makefile,
                                        const struct rule *pattern,
                                        const char *target, const char *stem)
{
    struct rule *rule = add_rule(makefile, target);
    rule->implicit = true;
    const struct string_list *dependency = pattern->dependencies;
    while (dependency)
    {
        char *name = substitute_pattern(dependency->value, stem);
        append_string(&rule->dependencies, name);
        free(name);
        dependency = dependency->next;
    }
    const struct string_list *command = pattern->commands;
    while (command)
    {
        append_string(&rule->commands, command->value);
        command = command->next;
    }
    return rule;
}

struct rule *resolve_pattern(struct makefile *makefile, const char *target)
{
    const struct rule *best = NULL;
    size_t best_size = 0;
    const struct rule *rule = makefile->rules;
    while (rule)
    {
        size_t size = 0;
        if (!rule->implicit && match_pattern(rule->target, target, &size)
            && (!best || size < best_size))
        {
            best = rule;
            best_size = size;
        }
        rule = rule->next;
    }
    if (!best)
        return NULL;
    size_t prefix_size = strchr(best->target, '%') - best->target;
    char *stem = copy_slice(target + prefix_size, best_size);
    struct rule *result = instantiate_pattern(makefile, best, target, stem);
    free(stem);
    return result;
}

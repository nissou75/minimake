#include "model.h"

#include <stdlib.h>
#include <string.h>

#include "memory.h"

struct rule *add_rule(struct makefile *makefile, const char *target)
{
    struct rule *rule = allocate(sizeof(*rule));

    *rule = (struct rule){ 0 };
    rule->target = copy_string(target);
    if (makefile->rules_tail)
        makefile->rules_tail->next = rule;
    else
        makefile->rules = rule;
    makefile->rules_tail = rule;
    return rule;
}

struct rule *find_rule(const struct makefile *makefile, const char *target)
{
    for (struct rule *rule = makefile->rules; rule; rule = rule->next)
    {
        if ((!strchr(rule->target, '%') || rule->implicit)
            && !strcmp(rule->target, target))
            return rule;
    }
    return NULL;
}

const char *find_default_target(const struct makefile *makefile)
{
    for (struct rule *rule = makefile->rules; rule; rule = rule->next)
    {
        if (!rule->implicit && !strchr(rule->target, '%')
            && strcmp(rule->target, ".PHONY"))
            return rule->target;
    }
    return NULL;
}

void mark_phony_rules(struct makefile *makefile)
{
    struct rule *declaration = find_rule(makefile, ".PHONY");

    if (!declaration)
        return;
    for (struct string_list *dep = declaration->dependencies; dep;
         dep = dep->next)
    {
        struct rule *rule = find_rule(makefile, dep->value);

        if (!rule)
        {
            rule = add_rule(makefile, dep->value);
            rule->implicit = true;
        }
        rule->phony = true;
    }
}

static void destroy_rule(struct rule *rule)
{
    free(rule->target);
    destroy_strings(rule->dependencies);
    destroy_strings(rule->commands);
    free(rule);
}

void destroy_makefile(struct makefile *makefile)
{
    struct rule *rule = makefile->rules;
    struct variable *variable = makefile->variables;

    while (rule)
    {
        struct rule *next = rule->next;

        destroy_rule(rule);
        rule = next;
    }
    while (variable)
    {
        struct variable *next = variable->next;

        free(variable->name);
        free(variable->value);
        free(variable);
        variable = next;
    }
    *makefile = (struct makefile){ 0 };
}

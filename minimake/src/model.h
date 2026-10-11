#ifndef MODEL_H
#define MODEL_H

#include <stdbool.h>
#include <stdio.h>

struct string_list
{
    char *value;
    struct string_list *next;
};

enum rule_state
{
    RULE_NEW,
    RULE_ACTIVE,
    RULE_DONE,
};

enum rule_result
{
    RESULT_BUILT,
    RESULT_CURRENT,
    RESULT_NOTHING,
};

struct rule
{
    char *target;
    struct string_list *dependencies;
    struct string_list *commands;
    struct rule *next;
    enum rule_state state;
    enum rule_result result;
    bool phony;
    bool implicit;
    bool changed;
};

struct variable
{
    char *name;
    char *value;
    struct variable *next;
};

struct makefile
{
    struct rule *rules;
    struct rule *rules_tail;
    struct variable *variables;
    struct variable *variables_tail;
};

void append_string(struct string_list **list, const char *value);
void destroy_strings(struct string_list *list);
char *join_strings(const struct string_list *list);
struct rule *add_rule(struct makefile *makefile, const char *target);
struct rule *find_rule(const struct makefile *makefile, const char *target);
const char *find_default_target(const struct makefile *makefile);
void mark_phony_rules(struct makefile *makefile);
void set_variable(struct makefile *makefile, const char *name,
                  const char *value);
const char *find_variable(const struct makefile *makefile, const char *name);
void destroy_makefile(struct makefile *makefile);
void print_makefile(const struct makefile *makefile, FILE *output);

#endif /* ! MODEL_H */

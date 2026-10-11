#include "build.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "execute.h"
#include "expand.h"
#include "memory.h"
#include "pattern.h"

struct changed_rule
{
    const struct rule *rule;
    struct changed_rule *next;
};

struct build_context
{
    struct makefile *makefile;
    struct changed_rule *changes;
    bool executed;
};

struct freshness
{
    struct stat target_info;
    bool target_exists;
    bool outdated;
    bool changed;
};

static int build_rule(struct build_context *context, struct rule *rule,
                      bool *changed);

static bool was_changed(const struct build_context *context,
                        const struct rule *rule)
{
    const struct changed_rule *change = context->changes;
    while (change)
    {
        if (change->rule == rule)
            return true;
        change = change->next;
    }
    return false;
}

static void record_change(struct build_context *context,
                          const struct rule *rule)
{
    struct changed_rule *change = allocate(sizeof(*change));
    change->rule = rule;
    change->next = context->changes;
    context->changes = change;
}

static void destroy_changes(struct changed_rule *change)
{
    while (change)
    {
        struct changed_rule *next = change->next;
        free(change);
        change = next;
    }
}

static int read_file_info(const char *name, struct stat *info)
{
    if (stat(name, info) == 0)
        return 0;
    if (errno == ENOENT || errno == ENOTDIR)
        return 1;
    fprintf(stderr, "minimake: cannot inspect '%s': ", name);
    perror(NULL);
    return 2;
}

static bool is_newer(const struct stat *dependency, const struct stat *target)
{
    if (dependency->st_mtim.tv_sec != target->st_mtim.tv_sec)
        return dependency->st_mtim.tv_sec > target->st_mtim.tv_sec;
    return dependency->st_mtim.tv_nsec > target->st_mtim.tv_nsec;
}

static int missing_target(const char *target, const struct rule *parent)
{
    if (parent)
        fprintf(stderr,
                "minimake: *** No rule to make target '%s', "
                "needed by '%s'.  Stop.\n",
                target, parent->target);
    else
        fprintf(stderr, "minimake: *** No rule to make target '%s'.  Stop.\n",
                target);
    return 2;
}

static int build_entry(struct build_context *context, const char *target,
                       const struct rule *parent, bool *changed)
{
    *changed = false;
    struct rule *rule = find_rule(context->makefile, target);
    if (!rule)
        rule = resolve_pattern(context->makefile, target);
    if (rule)
        return build_rule(context, rule, changed);
    struct stat info = { 0 };
    int status = read_file_info(target, &info);
    if (status == 0 || status == 2)
        return status;
    return missing_target(target, parent);
}

static int inspect_dependency(const char *name, struct freshness *freshness)
{
    struct stat info = { 0 };
    int status = read_file_info(name, &info);
    if (status == 2)
        return 2;
    if (status == 0 && freshness->target_exists
        && is_newer(&info, &freshness->target_info))
    {
        freshness->outdated = true;
        freshness->changed = true;
    }
    return 0;
}

static int build_dependencies(struct build_context *context, struct rule *rule,
                              struct freshness *freshness)
{
    const struct string_list *dependency = rule->dependencies;
    while (dependency)
    {
        bool changed = false;
        if (build_entry(context, dependency->value, rule, &changed) != 0)
            return 2;
        if (changed)
        {
            freshness->outdated = true;
            freshness->changed = true;
        }
        const struct rule *dependency_rule =
            find_rule(context->makefile, dependency->value);
        if ((!dependency_rule || !dependency_rule->phony)
            && inspect_dependency(dependency->value, freshness) != 0)
            return 2;
        dependency = dependency->next;
    }
    return 0;
}

static int execute_recipe(struct build_context *context,
                          const struct rule *rule, bool *executed)
{
    const struct string_list *command = rule->commands;
    while (command)
    {
        char *expanded = expand_string(context->makefile, command->value, rule);
        if (!expanded)
            return 2;
        int status = execute_command(expanded, executed);
        free(expanded);
        context->executed = context->executed || *executed;
        if (status != 0)
            return 2;
        command = command->next;
    }
    return 0;
}

static int finish_rule(struct build_context *context, struct rule *rule,
                       const struct freshness *freshness)
{
    rule->changed = freshness->changed || rule->phony;
    rule->result = rule->commands ? RESULT_CURRENT : RESULT_NOTHING;
    if (freshness->outdated && rule->commands)
    {
        bool executed = false;
        if (execute_recipe(context, rule, &executed) != 0)
            return 2;
        rule->changed = rule->changed || executed;
        rule->result = executed ? RESULT_BUILT : RESULT_NOTHING;
    }
    rule->state = RULE_DONE;
    if (rule->changed)
        record_change(context, rule);
    return 0;
}

static int build_rule(struct build_context *context, struct rule *rule,
                      bool *changed)
{
    if (rule->state == RULE_DONE)
    {
        *changed = rule->phony || was_changed(context, rule);
        return 0;
    }
    if (rule->state == RULE_ACTIVE)
    {
        fprintf(stderr,
                "minimake: *** Circular dependency involving '%s'. "
                "Stop.\n",
                rule->target);
        return 2;
    }
    rule->state = RULE_ACTIVE;
    struct freshness freshness = { 0 };
    int status =
        rule->phony ? 1 : read_file_info(rule->target, &freshness.target_info);
    if (status == 2)
        return 2;
    freshness.target_exists = status == 0 && !rule->phony;
    freshness.outdated = !freshness.target_exists;
    if (build_dependencies(context, rule, &freshness) != 0
        || finish_rule(context, rule, &freshness) != 0)
        return 2;
    *changed = rule->changed;
    return 0;
}

static int build_named_target(struct makefile *makefile, const char *target,
                              bool report)
{
    struct build_context context = { makefile, NULL, false };
    bool changed = false;
    int status = build_entry(&context, target, NULL, &changed);
    destroy_changes(context.changes);
    if (status != 0)
        return 2;
    if (context.executed || !report)
        return 0;
    const struct rule *rule = find_rule(makefile, target);
    if (rule && (rule->phony || rule->result == RESULT_NOTHING))
        printf("minimake: Nothing to be done for '%s'.\n", target);
    else
        printf("minimake: '%s' is up to date.\n", target);
    return 0;
}

int build_target(struct makefile *makefile, const char *target)
{
    return build_named_target(makefile, target, true);
}

int build_target_silent(struct makefile *makefile, const char *target)
{
    return build_named_target(makefile, target, false);
}

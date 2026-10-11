#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "model.h"

static struct variable *get_variable(const struct makefile *makefile,
                                     const char *name)
{
    for (struct variable *var = makefile->variables; var; var = var->next)
    {
        if (!strcmp(var->name, name))
            return var;
    }
    return NULL;
}

void set_variable(struct makefile *makefile, const char *name,
                  const char *value)
{
    struct variable *variable = get_variable(makefile, name);
    char *copy = copy_string(value);

    if (!variable)
    {
        variable = allocate(sizeof(*variable));
        variable->name = copy_string(name);
        variable->value = NULL;
        variable->next = NULL;
        if (makefile->variables_tail)
            makefile->variables_tail->next = variable;
        else
            makefile->variables = variable;
        makefile->variables_tail = variable;
    }
    free(variable->value);
    variable->value = copy;
}

const char *find_variable(const struct makefile *makefile, const char *name)
{
    struct variable *variable = get_variable(makefile, name);
    const char *environment = NULL;

    if (variable)
        return variable->value;
    environment = getenv(name);
    return environment ? environment : "";
}

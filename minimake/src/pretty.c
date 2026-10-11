#include "model.h"

void print_makefile(const struct makefile *makefile, FILE *output)
{
    fputs("# variables\n", output);
    for (struct variable *var = makefile->variables; var; var = var->next)
        fprintf(output, "'%s' = '%s'\n", var->name, var->value);
    fputs("# rules\n", output);
    for (struct rule *rule = makefile->rules; rule; rule = rule->next)
    {
        if (rule->implicit)
            continue;
        fprintf(output, "(%s):", rule->target);
        for (struct string_list *dep = rule->dependencies; dep; dep = dep->next)
            fprintf(output, " [%s]", dep->value);
        fputc('\n', output);
        for (struct string_list *cmd = rule->commands; cmd; cmd = cmd->next)
            fprintf(output, "    '%s'\n", cmd->value);
    }
}

#define _POSIX_C_SOURCE 200809L

#include "parser.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "expand.h"
#include "memory.h"
#include "text.h"

struct parser
{
    struct makefile *makefile;
    struct rule *current;
    const char *filename;
    size_t line;
    bool ignored_rule;
};

static int report_syntax(const struct parser *parser, const char *message)
{
    fprintf(stderr, "minimake: %s:%zu: *** %s. Stop.\n", parser->filename,
            parser->line, message);
    return 2;
}

static void strip_comment(char *line)
{
    char *comment = strchr(line, '#');
    if (comment)
        *comment = '\0';
}

static void append_dependencies(struct rule *rule, char *text)
{
    while (*text)
    {
        text += strspn(text, " \t\r\v\f");
        size_t length = strcspn(text, " \t\r\v\f");
        if (!length)
            return;
        char *dependency = copy_slice(text, length);
        append_string(&rule->dependencies, dependency);
        free(dependency);
        text += length;
    }
}

static int parse_variable(struct parser *parser, const char *name, char *value)
{
    if (!valid_name(name))
        return report_syntax(parser, "Invalid variable name");
    set_variable(parser->makefile, name, skip_blanks(value));
    return 0;
}

static int parse_rule(struct parser *parser, const char *target,
                      const char *dependencies)
{
    parser->ignored_rule = !*target;
    if (parser->ignored_rule)
        return 0;
    if (!valid_name(target))
        return report_syntax(parser, "Invalid target name");
    char *expanded = expand_string(parser->makefile, dependencies, NULL);
    if (!expanded)
        return 2;
    parser->current = add_rule(parser->makefile, target);
    append_dependencies(parser->current, expanded);
    free(expanded);
    return 0;
}

static int parse_definition(struct parser *parser, char *line)
{
    char *separator = strpbrk(line, ":=");
    if (!separator)
        return report_syntax(parser, "Missing separator");
    char kind = *separator;
    *separator = '\0';
    char *name = expand_string(parser->makefile, trim_name(line), NULL);
    if (!name)
        return 2;
    char *trimmed_name = trim_name(name);
    int status = 0;
    if (kind == '=')
        status = parse_variable(parser, trimmed_name, separator + 1);
    else
        status = parse_rule(parser, trimmed_name, separator + 1);
    free(name);
    return status;
}

static int parse_line(struct parser *parser, char *line)
{
    size_t length = strlen(line);
    if (length && line[length - 1] == '\n')
        line[length - 1] = '\0';
    if (*line == '\t')
    {
        if (parser->ignored_rule)
            return 0;
        if (!parser->current)
            return report_syntax(parser, "Command outside of rule");
        char *command = skip_blanks(line);
        if (*command)
            append_string(&parser->current->commands, command);
        return 0;
    }
    strip_comment(line);
    line = skip_blanks(line);
    if (!*line)
        return 0;
    parser->current = NULL;
    parser->ignored_rule = false;
    return parse_definition(parser, line);
}

int parse_makefile(struct makefile *makefile, FILE *input, const char *filename)
{
    struct parser parser = { makefile, NULL, filename, 0, false };
    char *line = NULL;
    size_t capacity = 0;
    int status = 0;

    while (!status && getline(&line, &capacity, input) != -1)
    {
        ++parser.line;
        status = parse_line(&parser, line);
    }
    free(line);
    if (ferror(input))
        return report_syntax(&parser, "Cannot read makefile");
    return status;
}

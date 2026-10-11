#include "options.h"

#include <stdio.h>
#include <string.h>

static bool wants_help(int argc, char **argv)
{
    for (int index = 1; index < argc; index++)
    {
        if (!strcmp(argv[index], "--"))
            return false;
        if (!strcmp(argv[index], "-f") && index + 1 < argc)
            index++;
        else if (!strcmp(argv[index], "-h"))
            return true;
    }
    return false;
}

static int parse_argument(struct options *options, int *index, char **argv)
{
    const char *argument = argv[*index];

    if (!strcmp(argument, "-f"))
    {
        if (!argv[*index + 1])
        {
            fputs("minimake: option '-f' requires a filename\n", stderr);
            return 2;
        }
        append_string(&options->files, argv[++*index]);
    }
    else if (!strncmp(argument, "-f", 2) && argument[2])
        append_string(&options->files, argument + 2);
    else if (!strcmp(argument, "-p"))
        options->pretty = true;
    else if (argument[0] == '-' && argument[1])
    {
        fprintf(stderr, "minimake: unknown option '%s'\n", argument);
        return 2;
    }
    else
        append_string(&options->targets, argument);
    return 0;
}

int parse_options(struct options *options, int argc, char **argv)
{
    bool operands = false;

    for (int index = 1; index < argc; index++)
    {
        if (!argv[index][0])
        {
            fputs("minimake: *** empty string invalid as argument.  Stop.\n",
                  stderr);
            return 2;
        }
    }
    options->help = wants_help(argc, argv);
    if (options->help)
        return 0;
    for (int index = 1; index < argc; index++)
    {
        if (!operands && !strcmp(argv[index], "--"))
            operands = true;
        else if (operands)
            append_string(&options->targets, argv[index]);
        else if (parse_argument(options, &index, argv))
            return 2;
    }
    return 0;
}

void destroy_options(struct options *options)
{
    destroy_strings(options->files);
    destroy_strings(options->targets);
    *options = (struct options){ 0 };
}

void print_help(void)
{
    puts("Usage: minimake [-h] [-p] [-f file] [targets...]\n"
         "  -h       Display this help and exit.\n"
         "  -p       Print parsed definitions without executing recipes.\n"
         "  -f file  Read a makefile; repeat to read multiple files.");
}

#include "load.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "parser.h"

int select_makefiles(struct options *options)
{
    struct stat status;

    if (options->files)
        return 0;
    if (!stat("makefile", &status))
        append_string(&options->files, "makefile");
    else if (!stat("Makefile", &status))
        append_string(&options->files, "Makefile");
    else
    {
        if (options->targets)
            fprintf(stderr,
                    "minimake: *** No rule to make target '%s'.  Stop.\n",
                    options->targets->value);
        else
            fputs("minimake: *** No targets specified and no makefile found."
                  "  Stop.\n",
                  stderr);
        return 2;
    }
    return 0;
}

static int load_one(struct makefile *makefile, const char *filename,
                    struct string_list **missing, FILE *standard_input)
{
    bool from_stdin = !strcmp(filename, "-");
    FILE *input = from_stdin ? standard_input : fopen(filename, "r");
    int result = 0;

    if (!input)
    {
        int error = errno;

        fprintf(stderr, "minimake: %s: %s\n", filename, strerror(error));
        if (error != ENOENT)
            return 2;
        append_string(missing, filename);
        return 0;
    }
    if (from_stdin)
        rewind(input);
    result = parse_makefile(makefile, input, filename);
    if (!from_stdin && fclose(input))
    {
        fprintf(stderr, "minimake: %s: could not close makefile\n", filename);
        return 2;
    }
    return result;
}

int load_makefiles(struct makefile *makefile, const struct options *options,
                   struct string_list **missing, FILE *standard_input)
{
    for (struct string_list *file = options->files; file; file = file->next)
    {
        if (load_one(makefile, file->value, missing, standard_input))
            return 2;
    }
    return 0;
}

static int copy_standard_input(FILE *cached)
{
    char buffer[4096];
    size_t count = 0;

    while ((count = fread(buffer, 1, sizeof(buffer), stdin)) != 0)
    {
        if (fwrite(buffer, 1, count, cached) != count)
            return 2;
    }
    return ferror(stdin) || fflush(cached) == EOF ? 2 : 0;
}

int cache_standard_input(const struct options *options, FILE **cached)
{
    for (struct string_list *file = options->files; file; file = file->next)
    {
        if (strcmp(file->value, "-"))
            continue;
        *cached = tmpfile();
        if (!*cached || copy_standard_input(*cached))
        {
            fputs("minimake: could not read makefile from standard input\n",
                  stderr);
            return 2;
        }
        return 0;
    }
    return 0;
}

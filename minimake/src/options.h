#ifndef OPTIONS_H
#define OPTIONS_H

#include <stdbool.h>

#include "model.h"

struct options
{
    struct string_list *files;
    struct string_list *targets;
    bool help;
    bool pretty;
};

int parse_options(struct options *options, int argc, char **argv);
void destroy_options(struct options *options);
void print_help(void);

#endif /* ! OPTIONS_H */

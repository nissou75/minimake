#include <stdio.h>

#include "build.h"
#include "load.h"
#include "model.h"
#include "options.h"

static int build_targets(struct makefile *makefile,
                         const struct string_list *targets)
{
    for (; targets; targets = targets->next)
    {
        if (build_target(makefile, targets->value))
            return 2;
    }
    return 0;
}

static int run_makefile(struct makefile *makefile,
                        const struct options *options)
{
    const char *target = NULL;

    if (options->pretty)
    {
        print_makefile(makefile, stdout);
        return 0;
    }
    if (options->targets)
        return build_targets(makefile, options->targets);
    target = find_default_target(makefile);
    if (!target)
    {
        fputs("minimake: *** No targets.  Stop.\n", stderr);
        return 2;
    }
    return build_target(makefile, target);
}

int main(int argc, char **argv)
{
    struct options options = { 0 };
    struct makefile makefile = { 0 };
    int result = parse_options(&options, argc, argv);

    if (!result && options.help)
        print_help();
    else if (!result)
    {
        result = select_makefiles(&options);
        if (!result)
            result = prepare_makefiles(&makefile, &options);
        if (!result)
            result = run_makefile(&makefile, &options);
    }
    destroy_makefile(&makefile);
    destroy_options(&options);
    return result;
}

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "build.h"
#include "load.h"
#include "memory.h"

struct file_snapshot
{
    const char *name;
    struct stat info;
    bool exists;
    bool phony;
    struct file_snapshot *next;
};

static struct file_snapshot *snapshot_files(const struct makefile *makefile,
                                            const struct string_list *files)
{
    struct file_snapshot *snapshots = NULL;

    for (; files; files = files->next)
    {
        if (!strcmp(files->value, "-"))
            continue;
        struct file_snapshot *snapshot = allocate(sizeof(*snapshot));
        const struct rule *rule = find_rule(makefile, files->value);

        snapshot->name = files->value;
        snapshot->exists = stat(snapshot->name, &snapshot->info) == 0;
        snapshot->phony = rule && rule->phony;
        snapshot->next = snapshots;
        snapshots = snapshot;
    }
    return snapshots;
}

static bool file_changed(const struct file_snapshot *snapshot)
{
    struct stat current;
    bool exists = stat(snapshot->name, &current) == 0;

    if (snapshot->phony)
        return false;
    if (exists != snapshot->exists)
        return true;
    if (!exists)
        return false;
    return current.st_mtim.tv_sec != snapshot->info.st_mtim.tv_sec
        || current.st_mtim.tv_nsec != snapshot->info.st_mtim.tv_nsec
        || current.st_size != snapshot->info.st_size
        || current.st_ino != snapshot->info.st_ino;
}

static int remake_files(struct makefile *makefile,
                        struct file_snapshot *snapshots, bool *changed)
{
    int status = 0;

    for (struct file_snapshot *file = snapshots; file; file = file->next)
    {
        if (build_target_silent(makefile, file->name))
        {
            status = 2;
            break;
        }
    }
    while (snapshots)
    {
        struct file_snapshot *next = snapshots->next;

        *changed = *changed || file_changed(snapshots);
        free(snapshots);
        snapshots = next;
    }
    return status;
}

static int load_and_remake(struct makefile *makefile,
                           const struct options *options, bool *changed,
                           FILE *standard_input)
{
    struct string_list *missing = NULL;
    int status = load_makefiles(makefile, options, &missing, standard_input);

    if (!status && options->pretty && missing)
        status = 2;
    destroy_strings(missing);
    if (status || options->pretty)
        return status;
    mark_phony_rules(makefile);
    struct file_snapshot *snapshots = snapshot_files(makefile, options->files);

    return remake_files(makefile, snapshots, changed);
}

static int prepare_cached_files(struct makefile *makefile,
                                const struct options *options,
                                FILE *standard_input)
{
    for (size_t restart = 0; restart < 64; restart++)
    {
        bool changed = false;
        int status =
            load_and_remake(makefile, options, &changed, standard_input);

        if (status || !changed)
            return status;
        destroy_makefile(makefile);
    }
    fputs("minimake: *** Makefiles keep changing after rebuilding.  Stop.\n",
          stderr);
    return 2;
}

int prepare_makefiles(struct makefile *makefile, const struct options *options)
{
    FILE *standard_input = NULL;
    int status = cache_standard_input(options, &standard_input);

    if (!status)
        status = prepare_cached_files(makefile, options, standard_input);
    if (standard_input && fclose(standard_input))
    {
        fputs("minimake: could not close cached standard input\n", stderr);
        status = 2;
    }
    return status;
}

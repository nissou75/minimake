#include <criterion/criterion.h>
#include <criterion/redirect.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "build.h"
#include "model.h"

static void assert_file_text(const char *path, const char *expected)
{
    FILE *file = fopen(path, "r");
    cr_assert_not_null(file);
    char text[32] = { 0 };
    size_t size = fread(text, 1, sizeof(text) - 1, file);
    cr_assert_not(ferror(file));
    cr_assert_eq(fclose(file), 0);
    cr_assert_eq(size, strlen(expected));
    cr_assert_str_eq(text, expected);
}

Test(build, changes_propagate_through_recipe_less_rules_once)
{
    cr_redirect_stdout();
    char path[] = "/tmp/minimake-build-XXXXXX";
    int descriptor = mkstemp(path);
    cr_assert_neq(descriptor, -1);
    cr_assert_eq(write(descriptor, "old", 3), 3);
    cr_assert_eq(close(descriptor), 0);
    struct makefile makefile = { 0 };
    struct rule *target = add_rule(&makefile, path);
    append_string(&target->dependencies, "intermediate");
    append_string(&target->commands, "@printf x >> $@");
    struct rule *middle = add_rule(&makefile, "intermediate");
    append_string(&middle->dependencies, "generated-leaf");
    struct rule *leaf = add_rule(&makefile, "generated-leaf");
    append_string(&leaf->commands, "@true");
    cr_assert_eq(build_target(&makefile, path), 0);
    cr_assert(middle->changed);
    cr_assert_eq(build_target(&makefile, path), 0);
    assert_file_text(path, "oldx");
    cr_assert_eq(unlink(path), 0);
    destroy_makefile(&makefile);
}

Test(build, compares_nanosecond_modification_dates)
{
    cr_redirect_stdout();
    char target_name[] = "/tmp/minimake-target-XXXXXX";
    char source_name[] = "/tmp/minimake-source-XXXXXX";
    int target_fd = mkstemp(target_name);
    int source_fd = mkstemp(source_name);
    cr_assert_neq(target_fd, -1);
    cr_assert_neq(source_fd, -1);
    cr_assert_eq(close(target_fd), 0);
    cr_assert_eq(close(source_fd), 0);
    struct timespec times[2] = { { 100, 100 }, { 100, 100 } };
    cr_assert_eq(utimensat(AT_FDCWD, target_name, times, 0), 0);
    times[1].tv_nsec = 101;
    cr_assert_eq(utimensat(AT_FDCWD, source_name, times, 0), 0);
    struct makefile makefile = { 0 };
    struct rule *target = add_rule(&makefile, target_name);
    append_string(&target->dependencies, source_name);
    append_string(&target->commands, "@printf x >> $@");
    cr_assert_eq(build_target(&makefile, target_name), 0);
    assert_file_text(target_name, "x");
    cr_assert_eq(unlink(target_name), 0);
    cr_assert_eq(unlink(source_name), 0);
    destroy_makefile(&makefile);
}

Test(build, shared_changed_dependency_invalidates_both_parents)
{
    cr_redirect_stdout();
    char path[] = "/tmp/minimake-shared-XXXXXX";
    int descriptor = mkstemp(path);
    cr_assert_neq(descriptor, -1);
    cr_assert_eq(close(descriptor), 0);
    struct makefile makefile = { 0 };
    struct rule *all = add_rule(&makefile, "shared-all");
    append_string(&all->dependencies, "shared-first");
    append_string(&all->dependencies, path);
    struct rule *first = add_rule(&makefile, "shared-first");
    append_string(&first->dependencies, "shared-leaf");
    append_string(&first->commands, "@true");
    struct rule *second = add_rule(&makefile, path);
    append_string(&second->dependencies, "shared-leaf");
    append_string(&second->commands, "@printf x >> $@");
    struct rule *leaf = add_rule(&makefile, "shared-leaf");
    append_string(&leaf->commands, "@true");
    cr_assert_eq(build_target(&makefile, "shared-all"), 0);
    assert_file_text(path, "x");
    cr_assert_eq(unlink(path), 0);
    destroy_makefile(&makefile);
}

Test(build, prior_requested_target_is_current_for_new_parent)
{
    cr_redirect_stdout();
    char path[] = "/tmp/minimake-prior-XXXXXX";
    int descriptor = mkstemp(path);
    cr_assert_neq(descriptor, -1);
    cr_assert_eq(close(descriptor), 0);
    struct makefile makefile = { 0 };
    struct rule *target = add_rule(&makefile, path);
    append_string(&target->dependencies, "prior-leaf");
    append_string(&target->commands, "@printf x >> $@");
    struct rule *leaf = add_rule(&makefile, "prior-leaf");
    append_string(&leaf->commands, "@true");
    cr_assert_eq(build_target(&makefile, "prior-leaf"), 0);
    cr_assert_eq(build_target(&makefile, path), 0);
    assert_file_text(path, "");
    cr_assert_eq(unlink(path), 0);
    destroy_makefile(&makefile);
}

Test(build, makefile_remake_does_not_report_target_status)
{
    cr_redirect_stdout();
    struct makefile makefile = { 0 };
    add_rule(&makefile, "empty-remake-target");
    cr_assert_eq(build_target_silent(&makefile, "empty-remake-target"), 0);
    fflush(stdout);
    cr_assert_stdout_eq_str("");
    destroy_makefile(&makefile);
}

Test(build, empty_expanded_recipe_has_nothing_to_do)
{
    cr_redirect_stdout();
    struct makefile makefile = { 0 };
    struct rule *target = add_rule(&makefile, "empty-expanded-recipe");
    append_string(&target->commands, "$(UNDEFINED_MINIMAKE_UNIT_VARIABLE)");
    cr_assert_eq(build_target(&makefile, target->target), 0);
    fflush(stdout);
    cr_assert_stdout_eq_str("minimake: Nothing to be done for "
                            "'empty-expanded-recipe'.\n");
    cr_assert_not(target->changed);
    destroy_makefile(&makefile);
}

Test(build, concrete_percent_target_is_built_once)
{
    cr_redirect_stdout();
    struct makefile makefile = { 0 };
    struct rule *pattern = add_rule(&makefile, "%.imaginary");
    append_string(&pattern->commands, "@true");
    cr_assert_eq(build_target(&makefile, "literal%.imaginary"), 0);
    cr_assert_eq(build_target(&makefile, "literal%.imaginary"), 0);
    cr_assert_not_null(pattern->next);
    cr_assert_null(pattern->next->next);
    fflush(stdout);
    cr_assert_stdout_eq_str("minimake: 'literal%.imaginary' is up to date.\n");
    destroy_makefile(&makefile);
}

Test(build, phony_ignores_existing_file_and_runs_once)
{
    cr_redirect_stdout();
    char path[] = "/tmp/minimake-phony-XXXXXX";
    int descriptor = mkstemp(path);
    cr_assert_neq(descriptor, -1);
    cr_assert_eq(close(descriptor), 0);
    struct makefile makefile = { 0 };
    struct rule *target = add_rule(&makefile, path);
    target->phony = true;
    append_string(&target->commands, "@printf x >> $@");
    cr_assert_eq(build_target(&makefile, path), 0);
    cr_assert_eq(build_target(&makefile, path), 0);
    assert_file_text(path, "x");
    cr_assert_eq(unlink(path), 0);
    destroy_makefile(&makefile);
}

Test(build, explicit_target_has_priority_over_pattern)
{
    cr_redirect_stdout();
    struct makefile makefile = { 0 };
    struct rule *pattern = add_rule(&makefile, "%.imaginary");
    append_string(&pattern->commands, "@false");
    struct rule *target = add_rule(&makefile, "example.imaginary");
    append_string(&target->commands, "@true");
    cr_assert_eq(build_target(&makefile, "example.imaginary"), 0);
    cr_assert_eq(target->result, RESULT_BUILT);
    cr_assert_eq(pattern->state, RULE_NEW);
    destroy_makefile(&makefile);
}

Test(build, command_failure_stops_following_recipes)
{
    cr_redirect_stdout();
    cr_redirect_stderr();
    struct makefile makefile = { 0 };
    struct rule *target = add_rule(&makefile, "failure.imaginary");
    append_string(&target->commands, "@false");
    append_string(&target->commands, "@echo should-not-run");
    cr_assert_eq(build_target(&makefile, "failure.imaginary"), 2);
    fflush(stdout);
    cr_assert_stdout_eq_str("");
    destroy_makefile(&makefile);
}

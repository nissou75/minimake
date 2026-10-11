#include <criterion/criterion.h>
#include <stdlib.h>

#include "model.h"
#include "pattern.h"

Test(pattern, matches_literal_prefix_and_suffix)
{
    size_t size = 0;
    cr_assert(match_pattern("src/%.o", "src/example.o", &size));
    cr_assert_eq(size, 7);
    cr_assert_not(match_pattern("src/%.o", "lib/example.o", &size));
    cr_assert_not(match_pattern("src/%.o", "src/example.c", &size));
    cr_assert_not(match_pattern("long%.suffix", "short", &size));
    cr_assert_not(match_pattern("file.o", "file.o", &size));
}

Test(pattern, only_first_percent_matches)
{
    size_t size = 0;
    cr_assert(match_pattern("%-%", "foo-%", &size));
    cr_assert_eq(size, 3);
    cr_assert_not(match_pattern("%-%", "foo-bar", &size));
    char *result = substitute_pattern("%-%", "foo");
    cr_assert_str_eq(result, "foo-%");
    free(result);
}

Test(pattern, substitution_preserves_dependencies_without_percent)
{
    char *result = substitute_pattern("common.h", "foo");
    cr_assert_str_eq(result, "common.h");
    free(result);
    result = substitute_pattern("src/%.c", "foo");
    cr_assert_str_eq(result, "src/foo.c");
    free(result);
}

Test(pattern, shortest_stem_wins_then_definition_order)
{
    struct makefile makefile = { 0 };
    struct rule *generic = add_rule(&makefile, "%.o");
    append_string(&generic->dependencies, "%.c");
    struct rule *first = add_rule(&makefile, "f%.o");
    append_string(&first->dependencies, "first/%.c");
    append_string(&first->commands, "@echo $@");
    struct rule *second = add_rule(&makefile, "%o.o");
    append_string(&second->dependencies, "second/%.f");
    struct rule *result = resolve_pattern(&makefile, "foo.o");
    cr_assert_not_null(result);
    cr_assert(result->implicit);
    cr_assert_str_eq(result->target, "foo.o");
    cr_assert_str_eq(result->dependencies->value, "first/oo.c");
    cr_assert_str_eq(result->commands->value, "@echo $@");
    cr_assert_str_eq(first->dependencies->value, "first/%.c");
    destroy_makefile(&makefile);
}

Test(pattern, instantiation_keeps_distinct_stems)
{
    struct makefile makefile = { 0 };
    struct rule *pattern = add_rule(&makefile, "%.o");
    append_string(&pattern->dependencies, "%.c");
    struct rule *first = resolve_pattern(&makefile, "first.o");
    struct rule *second = resolve_pattern(&makefile, "second.o");
    cr_assert_str_eq(first->dependencies->value, "first.c");
    cr_assert_str_eq(second->dependencies->value, "second.c");
    cr_assert_null(resolve_pattern(&makefile, "unmatched.txt"));
    destroy_makefile(&makefile);
}

Test(pattern, concrete_percent_never_becomes_another_wildcard)
{
    struct makefile makefile = { 0 };
    struct rule *generic = add_rule(&makefile, "%.o");
    append_string(&generic->commands, "@echo generic");
    struct rule *specific = add_rule(&makefile, "foobarX%");
    append_string(&specific->commands, "@echo specific");
    struct rule *first = resolve_pattern(&makefile, "foobar%.o");
    cr_assert_str_eq(first->commands->value, "@echo generic");
    struct rule *second = resolve_pattern(&makefile, "foobarX.o");
    cr_assert_str_eq(second->commands->value, "@echo specific");
    destroy_makefile(&makefile);
}

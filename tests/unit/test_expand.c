#define _POSIX_C_SOURCE 200809L

#include <criterion/criterion.h>
#include <stdlib.h>

#include "expand.h"
#include "model.h"

static void check_expansion(const struct makefile *makefile, const char *input,
                            const char *expected, const struct rule *rule)
{
    char *expanded = expand_string(makefile, input, rule);
    cr_assert_not_null(expanded);
    cr_assert_str_eq(expanded, expected);
    free(expanded);
}

Test(expand, supported_syntaxes)
{
    struct makefile makefile = { 0 };
    set_variable(&makefile, "A", "hello");
    check_expansion(&makefile, "$A $(A) ${A}", "hello hello hello", NULL);
    destroy_makefile(&makefile);
}

Test(expand, undefined_and_literal_dollars)
{
    struct makefile makefile = { 0 };
    check_expansion(&makefile, "$(MINIMAKE_UNDEFINED_9127)-$$A-$", "-$A-$",
                    NULL);
    check_expansion(&makefile, "'$$' \"$$\"", "'$' \"$\"", NULL);
    check_expansion(&makefile, "trailing$", "trailing$", NULL);
    set_variable(&makefile, "A$", "dollar name");
    set_variable(&makefile, "V", "text$");
    check_expansion(&makefile, "$(A$) $(V)", "dollar name text$", NULL);
    destroy_makefile(&makefile);
}

Test(expand, recursive_values_and_nested_names)
{
    struct makefile makefile = { 0 };
    set_variable(&makefile, "FOO", "$(BAR)");
    set_variable(&makefile, "BAR", "4");
    set_variable(&makefile, "4", "2");
    check_expansion(&makefile, "$(FOO)$($(FOO)) and $4", "42 and 2", NULL);
    check_expansion(&makefile, "${${FOO}}", "2", NULL);
    destroy_makefile(&makefile);
}

Test(expand, environment_fallback_and_override)
{
    struct makefile makefile = { 0 };
    cr_assert_eq(setenv("MINIMAKE_UNIT_ENV", "environment", 1), 0);
    check_expansion(&makefile, "$(MINIMAKE_UNIT_ENV)", "environment", NULL);
    set_variable(&makefile, "MINIMAKE_UNIT_ENV", "");
    check_expansion(&makefile, "$(MINIMAKE_UNIT_ENV)", "", NULL);
    cr_assert_eq(unsetenv("MINIMAKE_UNIT_ENV"), 0);
    destroy_makefile(&makefile);
}

Test(expand, environment_values_are_recursive)
{
    struct makefile makefile = { 0 };
    cr_assert_eq(setenv("MINIMAKE_UNIT_ENV", "$(A)", 1), 0);
    set_variable(&makefile, "A", "expanded");
    check_expansion(&makefile, "$(MINIMAKE_UNIT_ENV)", "expanded", NULL);
    cr_assert_eq(unsetenv("MINIMAKE_UNIT_ENV"), 0);
    destroy_makefile(&makefile);
}

Test(expand, automatic_variables_deduplicate_dependencies)
{
    struct makefile makefile = { 0 };
    struct rule *rule = add_rule(&makefile, "program");
    append_string(&rule->dependencies, "a.o");
    append_string(&rule->dependencies, "b.o");
    append_string(&rule->dependencies, "a.o");
    check_expansion(&makefile, "$@ $< $^", "program a.o a.o b.o", rule);
    check_expansion(&makefile, "$(@) ${<} $(^)", "program a.o a.o b.o", rule);
    destroy_makefile(&makefile);
}

Test(expand, automatic_variables_without_dependencies_or_context)
{
    struct makefile makefile = { 0 };
    struct rule *rule = add_rule(&makefile, "empty");
    check_expansion(&makefile, "$@-$<-$^", "empty--", rule);
    check_expansion(&makefile, "$@-$<-$^", "--", NULL);
    destroy_makefile(&makefile);
}

Test(expand, malformed_references)
{
    struct makefile makefile = { 0 };
    cr_assert_null(expand_string(&makefile, "$(A", NULL));
    cr_assert_null(expand_string(&makefile, "${A", NULL));
    cr_assert_null(expand_string(&makefile, "$($(A)", NULL));
    cr_assert_null(expand_string(&makefile, "$(A${B)})", NULL));
    cr_assert_null(expand_string(&makefile, "${A$(B})}", NULL));
}

Test(expand, recursive_cycles)
{
    struct makefile makefile = { 0 };
    set_variable(&makefile, "A", "$(B)");
    set_variable(&makefile, "B", "$(A)");
    cr_assert_null(expand_string(&makefile, "$(A)", NULL));
    destroy_makefile(&makefile);
}

Test(expand, mixed_delimiters_and_literal_parentheses)
{
    struct makefile makefile = { 0 };
    set_variable(&makefile, "KEY", "A");
    set_variable(&makefile, "A", "value");
    set_variable(&makefile, "A(B)", "parentheses");
    check_expansion(&makefile, "$( ${KEY})", "", NULL);
    check_expansion(&makefile, "$(A(B))", ")", NULL);
    check_expansion(&makefile, "$(A$(EMPTY)(B)) ${$(KEY)}", "parentheses value",
                    NULL);
    destroy_makefile(&makefile);
}

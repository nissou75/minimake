#include <criterion/criterion.h>

#include "options.h"

Test(options, flags_can_be_interspersed_and_all_files_are_loaded)
{
    struct options options = { 0 };
    char *arguments[] = { "minimake", "first", "-f",  "one", "-p",
                          "second",   "-f",    "two", NULL };

    cr_assert_eq(parse_options(&options, 8, arguments), 0);
    cr_assert(options.pretty);
    cr_assert_str_eq(options.files->value, "one");
    cr_assert_str_eq(options.files->next->value, "two");
    cr_assert_str_eq(options.targets->value, "first");
    cr_assert_str_eq(options.targets->next->value, "second");
    destroy_options(&options);
}

Test(options, help_overrides_invalid_options)
{
    struct options options = { 0 };
    char *arguments[] = { "minimake", "-unknown", "-h", "-f", NULL };

    cr_assert_eq(parse_options(&options, 4, arguments), 0);
    cr_assert(options.help);
    destroy_options(&options);
}

Test(options, filename_named_help_is_an_operand)
{
    struct options options = { 0 };
    char *arguments[] = { "minimake", "-f", "-h", NULL };

    cr_assert_eq(parse_options(&options, 3, arguments), 0);
    cr_assert_not(options.help);
    cr_assert_str_eq(options.files->value, "-h");
    destroy_options(&options);
}

Test(options, empty_argument_overrides_help)
{
    struct options options = { 0 };
    char *arguments[] = { "minimake", "-h", "", NULL };

    cr_assert_eq(parse_options(&options, 3, arguments), 2);
    destroy_options(&options);
}

Test(options, missing_filename_and_unknown_flag_are_errors)
{
    struct options options = { 0 };
    char *missing[] = { "minimake", "-f", NULL };
    char *unknown[] = { "minimake", "-x", NULL };

    cr_assert_eq(parse_options(&options, 2, missing), 2);
    destroy_options(&options);
    cr_assert_eq(parse_options(&options, 2, unknown), 2);
    destroy_options(&options);
}

Test(options, end_of_options_and_attached_filename)
{
    struct options options = { 0 };
    char *arguments[] = { "minimake", "-fpath", "--", "-h", "-p", NULL };

    cr_assert_eq(parse_options(&options, 5, arguments), 0);
    cr_assert_not(options.help);
    cr_assert_not(options.pretty);
    cr_assert_str_eq(options.files->value, "path");
    cr_assert_str_eq(options.targets->value, "-h");
    cr_assert_str_eq(options.targets->next->value, "-p");
    destroy_options(&options);
}

#include <criterion/criterion.h>
#include <stdlib.h>
#include <string.h>

#include "model.h"
#include "text.h"

Test(model, variable_redefinition_preserves_order_and_copies_input)
{
    struct makefile makefile = { 0 };
    char value[] = "initial";

    set_variable(&makefile, "FIRST", value);
    set_variable(&makefile, "SECOND", "second");
    value[0] = 'X';
    cr_assert_str_eq(find_variable(&makefile, "FIRST"), "initial");
    set_variable(&makefile, "FIRST", "updated");
    cr_assert_str_eq(find_variable(&makefile, "FIRST"), "updated");
    cr_assert_str_eq(makefile.variables->name, "FIRST");
    cr_assert_str_eq(makefile.variables->next->name, "SECOND");
    cr_assert_null(makefile.variables->next->next);
    destroy_makefile(&makefile);
}

Test(model, default_ignores_patterns_and_phony_declaration)
{
    struct makefile makefile = { 0 };

    add_rule(&makefile, "%.o");
    add_rule(&makefile, ".PHONY");
    add_rule(&makefile, "first");
    add_rule(&makefile, "second");
    cr_assert_str_eq(find_default_target(&makefile), "first");
    cr_assert_null(find_rule(&makefile, "%.o"));
    cr_assert_eq(find_rule(&makefile, "second"), makefile.rules_tail);
    destroy_makefile(&makefile);
}

Test(model, phony_marks_existing_and_creates_missing_targets)
{
    struct makefile makefile = { 0 };
    struct rule *declaration = add_rule(&makefile, ".PHONY");
    struct rule *clean = add_rule(&makefile, "clean");

    append_string(&declaration->dependencies, "clean");
    append_string(&declaration->dependencies, "absent");
    mark_phony_rules(&makefile);
    cr_assert(clean->phony);
    cr_assert(find_rule(&makefile, "absent")->phony);
    cr_assert_str_eq(find_default_target(&makefile), "clean");
    destroy_makefile(&makefile);
    cr_assert_null(makefile.rules);
    cr_assert_null(makefile.rules_tail);
}

Test(model, string_list_owns_strings_and_keeps_order)
{
    struct string_list *list = NULL;
    char first[] = "first";

    append_string(&list, first);
    append_string(&list, "second");
    first[0] = 'X';
    char *joined = join_strings(list);

    cr_assert_str_eq(joined, "first second");
    free(joined);
    destroy_strings(list);
}

Test(text, dynamic_buffer_grows_and_transfers_ownership)
{
    struct buffer buffer = { 0 };

    for (size_t index = 0; index < 4096; index++)
        append_character(&buffer, 'x');
    append_buffer(&buffer, "tail");
    char *result = take_buffer(&buffer);

    cr_assert_eq(strlen(result), 4100);
    cr_assert_str_eq(result + 4096, "tail");
    cr_assert_null(buffer.data);
    cr_assert_eq(buffer.capacity, 0);
    free(result);
}

Test(text, name_trimming_and_validation)
{
    char text[] = " \t name \t ";

    cr_assert_str_eq(trim_name(text), "name");
    cr_assert(valid_name("src/file.c"));
    cr_assert(valid_name(".PHONY"));
    cr_assert_not(valid_name(""));
    cr_assert_not(valid_name("two names"));
    cr_assert_not(valid_name("name:value"));
    cr_assert_not(valid_name("name=value"));
    cr_assert_not(valid_name("name#value"));
}

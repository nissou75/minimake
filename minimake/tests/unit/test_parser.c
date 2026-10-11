#include <criterion/criterion.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "expand.h"
#include "model.h"
#include "parser.h"

static int parse_text(struct makefile *makefile, const char *text)
{
    FILE *input = tmpfile();
    cr_assert_not_null(input);
    cr_assert_neq(fputs(text, input), EOF, "Cannot write parser fixture");
    rewind(input);
    int status = parse_makefile(makefile, input, "unit.makefile");
    cr_assert_eq(fclose(input), 0);
    return status;
}

Test(parser, variable_whitespace_and_comments)
{
    struct makefile makefile = { 0 };
    const char *text = " A \t = \t first  last  #comment\n";
    cr_assert_eq(parse_text(&makefile, text), 0);
    cr_assert_str_eq(find_variable(&makefile, "A"), "first  last  ");
    destroy_makefile(&makefile);
}

Test(parser, dependencies_expanded_before_tokenizing)
{
    struct makefile makefile = { 0 };
    const char *text = "LIST = first second\nall: before $(LIST) after\n";
    cr_assert_eq(parse_text(&makefile, text), 0);
    struct rule *rule = find_rule(&makefile, "all");
    cr_assert_not_null(rule);
    char *dependencies = join_strings(rule->dependencies);
    cr_assert_str_eq(dependencies, "before first second after");
    free(dependencies);
    destroy_makefile(&makefile);
}

Test(parser, early_dependency_and_late_recipe_expansion)
{
    struct makefile makefile = { 0 };
    const char *text = "all: $(LATER)\n\techo $(LATER)\nLATER=late\n";
    cr_assert_eq(parse_text(&makefile, text), 0);
    struct rule *rule = find_rule(&makefile, "all");
    cr_assert_not_null(rule);
    cr_assert_null(rule->dependencies);
    cr_assert_str_eq(rule->commands->value, "echo $(LATER)");
    char *command = expand_string(&makefile, rule->commands->value, rule);
    cr_assert_str_eq(command, "echo late");
    free(command);
    destroy_makefile(&makefile);
}

Test(parser, expanded_target_and_variable_names)
{
    struct makefile makefile = { 0 };
    const char *text = "NAME=FOO\n$(NAME)=all \n$(FOO):\nA$$=text$\n";
    cr_assert_eq(parse_text(&makefile, text), 0);
    cr_assert_str_eq(find_variable(&makefile, "FOO"), "all ");
    cr_assert_str_eq(find_variable(&makefile, "A$"), "text$");
    char *dollar_value = expand_string(&makefile, "$(A$)", NULL);
    cr_assert_str_eq(dollar_value, "text$");
    free(dollar_value);
    cr_assert_not_null(find_rule(&makefile, "all"));
    destroy_makefile(&makefile);
}

Test(parser, recipe_spacing_comments_and_blank_lines)
{
    struct makefile makefile = { 0 };
    const char *text = "all:\n\t   echo first  \n#comment\n\n"
                       "\t\n\t  #shell comment\n\techo second\n";
    cr_assert_eq(parse_text(&makefile, text), 0);
    struct string_list *commands = makefile.rules->commands;
    cr_assert_str_eq(commands->value, "echo first  ");
    cr_assert_str_eq(commands->next->value, "#shell comment");
    cr_assert_str_eq(commands->next->next->value, "echo second");
    cr_assert_null(commands->next->next->next);
    destroy_makefile(&makefile);
}

Test(parser, empty_targets_ignore_their_recipes)
{
    struct makefile makefile = { 0 };
    const char *text = ": ignored\n\techo ignored\nall:\n\techo kept\n";
    cr_assert_eq(parse_text(&makefile, text), 0);
    cr_assert_str_eq(makefile.rules->target, "all");
    cr_assert_null(makefile.rules->next);
    cr_assert_str_eq(makefile.rules->commands->value, "echo kept");
    destroy_makefile(&makefile);
}

Test(parser, definitions_end_recipe_context)
{
    struct makefile makefile = { 0 };
    cr_assert_eq(parse_text(&makefile, "all:\nA=1\n\techo invalid\n"), 2);
    destroy_makefile(&makefile);
    cr_assert_eq(parse_text(&makefile, "\techo invalid\n"), 2);
    destroy_makefile(&makefile);
}

Test(parser, invalid_definitions_and_unterminated_expansion)
{
    struct makefile makefile = { 0 };
    cr_assert_eq(parse_text(&makefile, "missing separator\n"), 2);
    destroy_makefile(&makefile);
    cr_assert_eq(parse_text(&makefile, "two targets:\n"), 2);
    destroy_makefile(&makefile);
    cr_assert_eq(parse_text(&makefile, "all: $(MISSING\n"), 2);
    destroy_makefile(&makefile);
}

Test(parser, no_final_newline_and_multiple_input_files)
{
    struct makefile makefile = { 0 };
    cr_assert_eq(parse_text(&makefile, "A=first\nall:"), 0);
    cr_assert_eq(parse_text(&makefile, "A=second\nother:\n\techo $(A)"), 0);
    cr_assert_str_eq(find_variable(&makefile, "A"), "second");
    cr_assert_str_eq(makefile.rules->next->target, "other");
    destroy_makefile(&makefile);
}

Test(parser, arbitrarily_long_lines)
{
    struct makefile makefile = { 0 };
    size_t length = 65536;
    char *text = malloc(length + 3);
    cr_assert_not_null(text);
    text[0] = 'A';
    text[1] = '=';
    memset(text + 2, 'x', length);
    text[length + 2] = '\0';
    cr_assert_eq(parse_text(&makefile, text), 0);
    cr_assert_eq(strlen(find_variable(&makefile, "A")), length);
    free(text);
    destroy_makefile(&makefile);
}

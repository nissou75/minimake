#!/bin/sh

recipe_outside_rule()
{
    printf '\tprintf "outside\\n"\n' >Makefile
    run
    expect_error
    expect_absent outside
}

recipe_after_variable_is_invalid()
{
    printf 'VALUE=text\n\t@printf "outside\\n"\n' >Makefile
    run
    expect_error
}

invalid_definition()
{
    printf 'this is not a definition\n' >Makefile
    run
    expect_error
    expect_empty_stdout
}

empty_target_is_ignored()
{
    cat >Makefile <<'EOF'
: ignored
all:
	@printf 'all\n'
EOF
    run
    expect_success
    expect_stdout <<'EOF'
all
EOF
}

variable_and_rule_whitespace()
{
    printf '  VALUE \t = \t hello world\n  all \t : \t dep \t\n' >Makefile
    printf '\t  @printf "<%%s>\\n" "$(VALUE)"\ndep:\n' >>Makefile
    run
    expect_success
    expect_stdout <<'EOF'
<hello world>
EOF
}

variable_reference_syntaxes()
{
    cat >Makefile <<'EOF'
A=one
B=two
C=three
all:
	@printf '<%s> <%s> <%s> <%s> <%s>\n' '$(A)' '${B}' '$C' '$(MINIMAKE_UNDEFINED_FOR_TEST)' '$$'
EOF
    unset MINIMAKE_UNDEFINED_FOR_TEST
    run
    expect_success
    expect_stdout <<'EOF'
<one> <two> <three> <> <$>
EOF
}

recipe_variables_defined_later()
{
    cat >Makefile <<'EOF'
all:
	@printf '%s\n' '$(LATER)'
LATER=available
EOF
    run
    expect_success
    expect_stdout <<'EOF'
available
EOF
}

dependencies_expanded_at_parse_time()
{
    cat >Makefile <<'EOF'
all: $(MINIMAKE_LATE_DEPENDENCY)
	@printf 'all\n'
MINIMAKE_LATE_DEPENDENCY=unbuildable
EOF
    unset MINIMAKE_LATE_DEPENDENCY
    run
    expect_success
    expect_stdout <<'EOF'
all
EOF
}

target_and_dependency_expansion()
{
    cat >Makefile <<'EOF'
TARGET=chosen
DEPS=left right
$(TARGET): $(DEPS)
	@printf '%s\n' '$@:$^'
TARGET=changed
DEPS=missing
left:
	@printf 'left\n'
right:
	@printf 'right\n'
EOF
    run chosen
    expect_success
    expect_stdout <<'EOF'
left
right
chosen:left right
EOF
}

expanded_variable_name()
{
    cat >Makefile <<'EOF'
NAME=ANSWER
$(NAME)=forty two
all:
	@printf '%s\n' '$(ANSWER)'
EOF
    run
    expect_success
    expect_stdout <<'EOF'
forty two
EOF
}

recursive_and_nested_variables()
{
    cat >Makefile <<'EOF'
FOO=$(BAR)
all:
	@printf 'The answer is %s%s and not %s\n' '$(FOO)' '$($(FOO))' '$4'
BAR=4
$(FOO)=2
EOF
    run
    expect_success
    expect_stdout <<'EOF'
The answer is 42 and not 2
EOF
}

recursive_parse_time_expansion()
{
    cat >Makefile <<'EOF'
DEPS=$(INNER)
INNER=left right
all: ${DEPS}
	@printf 'all\n'
left:
	@printf 'left\n'
right:
	@printf 'right\n'
EOF
    run
    expect_success
    expect_stdout <<'EOF'
left
right
all
EOF
}

environment_fallback_and_override()
{
    MINIMAKE_TEST_ENV=environment
    MINIMAKE_TEST_OVERRIDE=environment
    export MINIMAKE_TEST_ENV MINIMAKE_TEST_OVERRIDE
    cat >Makefile <<'EOF'
MINIMAKE_TEST_OVERRIDE=makefile
all:
	@printf '%s %s\n' '$(MINIMAKE_TEST_ENV)' '$(MINIMAKE_TEST_OVERRIDE)'
EOF
    run
    expect_success
    expect_stdout <<'EOF'
environment makefile
EOF
}

empty_makefile_variable_overrides_environment()
{
    MINIMAKE_TEST_ENV=environment
    export MINIMAKE_TEST_ENV
    cat >Makefile <<'EOF'
MINIMAKE_TEST_ENV=
all:
	@printf '<%s>\n' '$(MINIMAKE_TEST_ENV)'
EOF
    run
    expect_success
    expect_stdout <<'EOF'
<>
EOF
}

environment_dependency_expansion()
{
    MINIMAKE_TEST_DEPS='left right'
    export MINIMAKE_TEST_DEPS
    cat >Makefile <<'EOF'
all: $(MINIMAKE_TEST_DEPS)
	@printf '%s\n' '$^'
left:
right:
EOF
    run
    expect_success
    expect_stdout <<'EOF'
left right
EOF
}

single_quotes_do_not_disable_expansion()
{
    cat >Makefile <<'EOF'
VALUE=expanded
all:
	@printf '%s\n' '$(VALUE)'
EOF
    run
    expect_success
    expect_stdout <<'EOF'
expanded
EOF
}

unclosed_reference_in_target()
{
    printf '$(UNCLOSED:\n\t@printf "never\\n"\n' >Makefile
    run
    expect_error
    expect_empty_stdout
}

unclosed_reference_in_recipe()
{
    cat >Makefile <<'EOF'
all:
	@printf '%s\n' '${UNCLOSED'
	@touch should_not_exist
EOF
    run
    expect_error
    expect_empty_stdout
    expect_absent should_not_exist
}

pretty_print_exact_format()
{
    cat >input.mk <<'EOF'
# ignored comment
FIRST = hello # discarded
initial: one two # discarded
	  @printf '%s\n' '$(FIRST)'
SECOND=world
next:
	 # shell comment
EOF
    printf 'TRAILING=kept  \n' >>input.mk
    printf 'last:\n\t  echo trailing  \n' >>input.mk
    run next -f input.mk -p
    expect_success
    {
        cat <<'EOF'
# variables
'FIRST' = 'hello '
'SECOND' = 'world'
EOF
        printf "'TRAILING' = 'kept  '\n"
        cat <<'EOF'
# rules
(initial): [one] [two]
    '@printf '%s\n' '$(FIRST)''
(next):
    '# shell comment'
(last):
EOF
        printf "    'echo trailing  '\n"
    } >expected.stdout
    cmp -s expected.stdout "$OUT" || fail 'Pretty-print format differs'
}

pretty_print_does_not_execute()
{
    cat >Makefile <<'EOF'
A=raw
all:
	@touch should_not_exist
	false
EOF
    run all -p
    expect_success
    expect_absent should_not_exist
    expect_stdout <<'EOF'
# variables
'A' = 'raw'
# rules
(all):
    '@touch should_not_exist'
    'false'
EOF
}

pretty_print_keeps_recursive_values_raw()
{
    cat >Makefile <<'EOF'
FIRST=$(LATER)
LATER=expanded
all:
	@printf '%s\n' '$(FIRST)'
EOF
    run -p
    expect_success
    expect_stdout <<'EOF'
# variables
'FIRST' = '$(LATER)'
'LATER' = 'expanded'
# rules
(all):
    '@printf '%s\n' '$(FIRST)''
EOF
}

blank_lines_and_comments_inside_recipe()
{
    cat >Makefile <<'EOF'
all:
	@printf 'first\n'

# parser comment between recipe lines
   # another parser comment
	@printf 'second\n'
next:
	@printf 'not requested\n'
EOF
    run all
    expect_success
    expect_stdout <<'EOF'
first
second
EOF
}

recipe_hash_comments_are_logged()
{
    cat >Makefile <<'EOF'
VALUE=test # discarded
all: # discarded
	echo a $(VALUE) line # shell comment
	# shell-only comment
EOF
    run
    expect_success
    expect_stdout <<'EOF'
echo a test  line # shell comment
a test line
# shell-only comment
EOF
}

arbitrarily_long_lines()
{
    awk 'BEGIN { for (i = 0; i < 20000; i++) printf "x" }' >long_value
    {
        printf 'VALUE='
        cat long_value
        printf '\nall:\n\t@printf '\''%%s'\'' '\''$(VALUE)'\'' >result\n'
    } >Makefile
    run
    expect_success
    expect_empty_stdout
    cmp -s long_value result || fail 'Long variable or recipe was truncated'
}

final_line_without_newline()
{
    printf 'all:\n\t@printf "last line\\n"' >Makefile
    run
    expect_success
    expect_stdout <<'EOF'
last line
EOF
}

direct_trailing_dollar_is_preserved()
{
    cat >Makefile <<'EOF'
all:
	@echo direct$
EOF
    run
    expect_success
    expect_stdout <<'EOF'
direct$
EOF
}

variable_trailing_dollar_is_preserved()
{
    cat >Makefile <<'EOF'
VALUE=variable$
all:
	@printf '%s\n' '$(VALUE)'
EOF
    run
    expect_success
    expect_stdout <<'EOF'
variable$
EOF
}

variable_escaped_dollar_is_not_expanded_twice()
{
    MINIMAKE_TEST_DOLLAR=wrong
    export MINIMAKE_TEST_DOLLAR
    cat >Makefile <<'EOF'
TRAILING=trailing$$
VALUE=$$MINIMAKE_TEST_DOLLAR
all:
	@printf '%s\n' '$(TRAILING)' '$(VALUE)'
EOF
    run
    expect_success
    expect_stdout <<'EOF'
trailing$
$MINIMAKE_TEST_DOLLAR
EOF
}

run_case recipe_outside_rule
run_case recipe_after_variable_is_invalid
run_case invalid_definition
run_case empty_target_is_ignored
run_case variable_and_rule_whitespace
run_case variable_reference_syntaxes
run_case recipe_variables_defined_later
run_case dependencies_expanded_at_parse_time
run_case target_and_dependency_expansion
run_case expanded_variable_name
run_case recursive_and_nested_variables
run_case recursive_parse_time_expansion
run_case environment_fallback_and_override
run_case empty_makefile_variable_overrides_environment
run_case environment_dependency_expansion
run_case single_quotes_do_not_disable_expansion
run_case unclosed_reference_in_target
run_case unclosed_reference_in_recipe
run_case pretty_print_exact_format
run_case pretty_print_does_not_execute
run_case pretty_print_keeps_recursive_values_raw
run_case blank_lines_and_comments_inside_recipe
run_case recipe_hash_comments_are_logged
run_case arbitrarily_long_lines
run_case final_line_without_newline
run_case direct_trailing_dollar_is_preserved
run_case variable_trailing_dollar_is_preserved
run_case variable_escaped_dollar_is_not_expanded_twice

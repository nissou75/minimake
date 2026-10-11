#!/bin/sh

help_without_makefile()
{
    run -h
    expect_success
    [ -s "$OUT" ] || fail 'Help must print a message'
}

help_has_priority()
{
    run nonexistent -f absent.mk -p --unknown -h
    expect_success
    [ -s "$OUT" ] || fail 'Help must print a message'
}

help_beats_missing_file_argument()
{
    run -h -f
    expect_success
    [ -s "$OUT" ] || fail 'Help must print a message'
}

help_consumed_as_filename()
{
    run -f -h
    expect_error
    expect_empty_stdout
}

help_named_file()
{
    printf 'all:\n\t@printf "filename\\n"\n' >./-h
    run -f -h
    expect_success
    expect_stdout <<'EOF'
filename
EOF
}

empty_argument_beats_help()
{
    run '' -h all
    expect_status 2
    expect_empty_stdout
    expect_stderr <<'EOF'
minimake: *** empty string invalid as argument.  Stop.
EOF
}

empty_argument_after_help()
{
    run -h ''
    expect_status 2
    expect_empty_stdout
    expect_stderr <<'EOF'
minimake: *** empty string invalid as argument.  Stop.
EOF
}

missing_file_argument()
{
    run -f
    expect_error
    expect_empty_stdout
}

unknown_option()
{
    run --unknown
    expect_error
    expect_empty_stdout
}

no_makefile_no_target()
{
    run
    expect_status 2
    expect_empty_stdout
    expect_stderr <<'EOF'
minimake: *** No targets specified and no makefile found.  Stop.
EOF
}

no_makefile_explicit_target()
{
    run missing
    expect_status 2
    expect_empty_stdout
    expect_stderr <<'EOF'
minimake: *** No rule to make target 'missing'.  Stop.
EOF
}

lowercase_makefile_preferred()
{
    printf 'all:\n\t@printf "lowercase\\n"\n' >makefile
    printf 'all:\n\t@printf "uppercase\\n"\n' >Makefile
    run
    expect_success
    expect_stdout <<'EOF'
lowercase
EOF
}

uppercase_makefile_fallback()
{
    printf 'all:\n\t@printf "uppercase\\n"\n' >Makefile
    run
    expect_success
    expect_stdout <<'EOF'
uppercase
EOF
}

explicit_makefile_selection()
{
    printf 'all:\n\t@printf "default\\n"\n' >makefile
    printf 'all:\n\t@printf "selected\\n"\n' >selected.mk
    run -f selected.mk
    expect_success
    expect_stdout <<'EOF'
selected
EOF
}

interleaved_options_and_targets()
{
    cat >commands.mk <<'EOF'
first:
	@printf 'first\n'
second:
	@printf 'second\n'
EOF
    run second -f commands.mk first
    expect_success
    expect_stdout <<'EOF'
second
first
EOF
}

default_ignores_pattern_and_phony_declaration()
{
    cat >Makefile <<'EOF'
.PHONY: first
%.generated:
	@printf 'pattern\n'
first:
	@printf 'first\n'
second:
	@printf 'second\n'
EOF
    run
    expect_success
    expect_stdout <<'EOF'
first
EOF
}

only_patterns_no_default()
{
    cat >Makefile <<'EOF'
VALUE = text
%.o:
	@printf 'pattern\n'
EOF
    run
    expect_status 2
    expect_empty_stdout
    expect_stderr <<'EOF'
minimake: *** No targets.  Stop.
EOF
}

unknown_explicit_target()
{
    printf 'all:\n' >Makefile
    run absent
    expect_status 2
    expect_empty_stdout
    expect_stderr <<'EOF'
minimake: *** No rule to make target 'absent'.  Stop.
EOF
}

missing_dependency_diagnostic()
{
    cat >Makefile <<'EOF'
all: absent
	@printf 'should not run\n'
EOF
    run all
    expect_status 2
    expect_empty_stdout
    expect_stderr <<'EOF'
minimake: *** No rule to make target 'absent', needed by 'all'.  Stop.
EOF
}

nested_missing_dependency_diagnostic()
{
    cat >Makefile <<'EOF'
all: intermediate
	@printf 'should not run\n'
intermediate: absent
EOF
    run all
    expect_status 2
    expect_empty_stdout
    expect_stderr <<'EOF'
minimake: *** No rule to make target 'absent', needed by 'intermediate'.  Stop.
EOF
}

multiple_makefiles_are_loaded()
{
    cat >first.mk <<'EOF'
WORD=combined
first: second
	@printf '%s first\n' '$(WORD)'
EOF
    cat >second.mk <<'EOF'
second:
	@printf '%s second\n' '$(WORD)'
EOF
    run -f first.mk -f second.mk
    expect_success
    expect_stdout <<'EOF'
combined second
combined first
EOF
}

missing_makefile_built_before_default()
{
    cat >existing.mk <<'EOF'
first:
	@printf 'first\n'
generated.mk:
	@printf 'generated\n'
EOF
    run -f generated.mk -f existing.mk
    expect_status 0
    expect_stdout <<'EOF'
generated
first
EOF
    expect_stderr <<'EOF'
minimake: generated.mk: No such file or directory
EOF
}

missing_makefile_built_before_explicit_targets()
{
    cat >existing.mk <<'EOF'
first:
	@printf 'first\n'
second:
	@printf 'second\n'
generated.mk:
	@printf 'generated\n'
EOF
    run second -f generated.mk first -f existing.mk
    expect_status 0
    expect_stdout <<'EOF'
generated
second
first
EOF
    expect_stderr <<'EOF'
minimake: generated.mk: No such file or directory
EOF
}

multiple_missing_makefiles_remade_in_reverse_order()
{
    cat >existing.mk <<'EOF'
all:
	@printf 'all\n'
one.mk:
	@printf 'one\n'
two.mk:
	@printf 'two\n'
EOF
    run -f one.mk -f existing.mk -f two.mk all
    expect_status 0
    expect_stdout <<'EOF'
two
one
all
EOF
    expect_stderr <<'EOF'
minimake: one.mk: No such file or directory
minimake: two.mk: No such file or directory
EOF
}

unbuildable_missing_makefile()
{
    printf 'all:\n\t@printf "should not run\\n"\n' >existing.mk
    run -f absent.mk -f existing.mk
    expect_status 2
    expect_empty_stdout
    expect_stderr <<'EOF'
minimake: absent.mk: No such file or directory
minimake: *** No rule to make target 'absent.mk'.  Stop.
EOF
}

existing_makefile_can_be_updated()
{
    cat >selected.mk <<'EOF'
all:
	@printf 'all\n'
selected.mk: source
	@printf 'remade\n'
EOF
    : >source
    touch -t 200001010000 selected.mk
    touch -t 200001010001 source
    run -f selected.mk all
    expect_success
    expect_stdout <<'EOF'
remade
all
EOF
}

makefile_from_standard_input()
{
    cat >stdin.mk <<'EOF'
all:
	@printf 'standard input\n'
EOF
    run -f - <stdin.mk
    expect_success
    expect_stdout <<'EOF'
standard input
EOF
}

standard_input_pretty_print()
{
    cat >stdin.mk <<'EOF'
VALUE=raw
all:
	@printf '%s\n' '$(VALUE)'
EOF
    run -p -f - <stdin.mk
    expect_success
    expect_stdout <<'EOF'
# variables
'VALUE' = 'raw'
# rules
(all):
    '@printf '%s\n' '$(VALUE)''
EOF
}

standard_input_merges_with_other_makefile()
{
    cat >stdin.mk <<'EOF'
PREFIX=standard
all: child
	@printf '%s %s\n' '$(PREFIX)' '$(SUFFIX)'
EOF
    cat >other.mk <<'EOF'
SUFFIX=input
child:
	@printf 'child\n'
EOF
    run -f - -f other.mk <stdin.mk
    expect_success
    expect_stdout <<'EOF'
child
standard input
EOF
}

standard_input_parsed_after_other_makefile()
{
    printf 'TARGET=all\n' >other.mk
    cat >stdin.mk <<'EOF'
$(TARGET):
	@printf 'parse order\n'
EOF
    run -f other.mk -f - <stdin.mk
    expect_success
    expect_stdout <<'EOF'
parse order
EOF
}

standard_input_is_cached_when_makefiles_reload()
{
    cat >stdin.mk <<'EOF'
WORD=cached stdin
all: generated_target
	@printf '%s\n' '$(WORD)'
generated.mk:
	@printf 'generated_target:\n\t@printf "generated target\\n"\n' >generated.mk
EOF
    run -f - -f generated.mk all <stdin.mk
    expect_status 0
    expect_stdout <<'EOF'
generated target
cached stdin
EOF
    expect_stderr <<'EOF'
minimake: generated.mk: No such file or directory
EOF
}

run_case help_without_makefile
run_case help_has_priority
run_case help_beats_missing_file_argument
run_case help_consumed_as_filename
run_case help_named_file
run_case empty_argument_beats_help
run_case empty_argument_after_help
run_case missing_file_argument
run_case unknown_option
run_case no_makefile_no_target
run_case no_makefile_explicit_target
run_case lowercase_makefile_preferred
run_case uppercase_makefile_fallback
run_case explicit_makefile_selection
run_case interleaved_options_and_targets
run_case default_ignores_pattern_and_phony_declaration
run_case only_patterns_no_default
run_case unknown_explicit_target
run_case missing_dependency_diagnostic
run_case nested_missing_dependency_diagnostic
run_case multiple_makefiles_are_loaded
run_case missing_makefile_built_before_default
run_case missing_makefile_built_before_explicit_targets
run_case multiple_missing_makefiles_remade_in_reverse_order
run_case unbuildable_missing_makefile
run_case existing_makefile_can_be_updated
run_case makefile_from_standard_input
run_case standard_input_pretty_print
run_case standard_input_merges_with_other_makefile
run_case standard_input_parsed_after_other_makefile
run_case standard_input_is_cached_when_makefiles_reload

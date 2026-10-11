#!/bin/sh

commands_logged_before_execution()
{
    cat >Makefile <<'EOF'
all:
	printf 'first\n'
	printf 'second\n'
EOF
    run
    expect_success
    expect_stdout <<'EOF'
printf 'first\n'
first
printf 'second\n'
second
EOF
}

command_whitespace_and_silence()
{
    printf 'all:\n\t   \tprintf "loud\\n"  \n' >Makefile
    printf '\t \t @  printf "silent\\n"\n' >>Makefile
    run
    expect_success
    {
        printf 'printf "loud\\n"  \n'
        printf 'loud\nsilent\n'
    } >expected.stdout
    cmp -s expected.stdout "$OUT" || fail 'Command whitespace or @ logging differs'
}

commands_use_separate_shells()
{
    unset MINIMAKE_TEST_SHELL_VAR
    cat >Makefile <<'EOF'
all:
	@MINIMAKE_TEST_SHELL_VAR=first
	@printf '<%s>\n' "$$MINIMAKE_TEST_SHELL_VAR"
	@MINIMAKE_TEST_SHELL_VAR=second; printf '<%s>\n' "$$MINIMAKE_TEST_SHELL_VAR"
EOF
    run
    expect_success
    expect_stdout <<'EOF'
<>
<second>
EOF
}

shell_builtins_and_external_commands()
{
    cat >Makefile <<'EOF'
all:
	@if test ! -f data; then printf 'shell and external\n' >data; fi
	@cat data
EOF
    run
    expect_success
    expect_stdout <<'EOF'
shell and external
EOF
    expect_content data <<'EOF'
shell and external
EOF
}

recipe_failure_stops_all_building()
{
    cat >Makefile <<'EOF'
all: child
	@touch parent_should_not_exist
child:
	printf 'before failure\n'
	false
	@touch child_should_not_exist
later:
	@touch later_should_not_exist
EOF
    run all later
    expect_error
    expect_stdout <<'EOF'
printf 'before failure\n'
before failure
false
EOF
    expect_absent parent_should_not_exist
    expect_absent child_should_not_exist
    expect_absent later_should_not_exist
}

nonexistent_shell_command()
{
    cat >Makefile <<'EOF'
all:
	@./minimake_test_command_that_does_not_exist
	@touch should_not_exist
EOF
    run
    expect_error
    expect_absent should_not_exist
}

shell_terminated_by_signal()
{
    cat >Makefile <<'EOF'
all:
	@kill -TERM $$$$
	@touch should_not_exist
EOF
    run
    expect_error
    expect_empty_stdout
    expect_absent should_not_exist
}

dependencies_built_left_to_right_once()
{
    cat >Makefile <<'EOF'
all: left right shared
	@printf 'all\n'
left: shared
	@printf 'left\n'
right: shared
	@printf 'right\n'
shared:
	@printf 'shared\n'
EOF
    run all
    expect_success
    expect_stdout <<'EOF'
shared
left
right
all
EOF
}

repeated_target_not_rebuilt()
{
    cat >Makefile <<'EOF'
all:
	@printf 'once\n'
EOF
    run all all all
    expect_success
    expect_stdout <<'EOF'
once
minimake: 'all' is up to date.
minimake: 'all' is up to date.
EOF
}

dependency_then_explicit_target_not_rebuilt()
{
    cat >Makefile <<'EOF'
all: child
	@printf 'all\n'
child:
	@printf 'child\n'
EOF
    run all child
    expect_success
    expect_stdout <<'EOF'
child
all
minimake: 'child' is up to date.
EOF
}

empty_rules_nothing_to_do()
{
    cat >Makefile <<'EOF'
missing:
existing:
EOF
    : >existing
    run missing existing missing
    expect_success
    expect_stdout <<'EOF'
minimake: Nothing to be done for 'missing'.
minimake: Nothing to be done for 'existing'.
minimake: Nothing to be done for 'missing'.
EOF
}

existing_target_without_dependencies()
{
    cat >Makefile <<'EOF'
result:
	@printf 'should not run\n'
EOF
    : >result
    run result
    expect_success
    expect_stdout <<'EOF'
minimake: 'result' is up to date.
EOF
}

older_file_dependency_is_up_to_date()
{
    cat >Makefile <<'EOF'
result: source
	@printf 'should not run\n'
EOF
    : >source
    : >result
    touch -t 200001010000 source
    touch -t 200001010001 result
    run result
    expect_success
    expect_stdout <<'EOF'
minimake: 'result' is up to date.
EOF
}

equal_file_dates_are_up_to_date()
{
    cat >Makefile <<'EOF'
result: source
	@printf 'should not run\n'
EOF
    : >source
    : >result
    touch -t 200001010000 source result
    run result
    expect_success
    expect_stdout <<'EOF'
minimake: 'result' is up to date.
EOF
}

newer_file_dependency_rebuilds()
{
    cat >Makefile <<'EOF'
result: source
	@printf 'rebuilt\n' >result
	@cat result
EOF
    : >source
    : >result
    touch -t 200001010000 result
    touch -t 200001010001 source
    run result
    expect_success
    expect_stdout <<'EOF'
rebuilt
EOF
    expect_content result <<'EOF'
rebuilt
EOF
}

subsecond_file_dates_are_compared()
{
    command -v python3 >/dev/null 2>&1 || exit 77
    cat >Makefile <<'EOF'
result: source
	@printf 'rebuilt\n'
EOF
    python3 - <<'PY'
import os
from pathlib import Path

for filename, nanoseconds in (
    ("result", 946684800100000000),
    ("source", 946684800200000000),
):
    Path(filename).touch()
    os.utime(filename, ns=(nanoseconds, nanoseconds))
PY
    run result
    expect_success
    expect_stdout <<'EOF'
rebuilt
EOF
}

missing_dependency_file_is_built_first()
{
    cat >Makefile <<'EOF'
result: source
	@printf 'result\n'
source:
	@printf 'source\n' >source
	@cat source
EOF
    : >result
    touch -t 200001010000 result
    run result
    expect_success
    expect_stdout <<'EOF'
source
result
EOF
}

nothing_to_do_dependency_does_not_force_recipe()
{
    cat >Makefile <<'EOF'
result: empty
	@printf 'should not run\n'
empty:
EOF
    : >result
    run result
    expect_success
    expect_stdout <<'EOF'
minimake: 'result' is up to date.
EOF
}

rebuilt_virtual_dependency_forces_recipe()
{
    cat >Makefile <<'EOF'
result: virtual
	@printf 'result\n'
virtual:
	@printf 'virtual\n'
EOF
    : >result
    run result
    expect_success
    expect_stdout <<'EOF'
virtual
result
EOF
}

empty_parent_after_explicit_child()
{
    cat >Makefile <<'EOF'
all: child
child:
	@printf 'child\n'
EOF
    run child all
    expect_success
    expect_stdout <<'EOF'
child
minimake: Nothing to be done for 'all'.
EOF
}

empty_parent_after_child_build()
{
    cat >Makefile <<'EOF'
all: child
child:
	@printf 'child\n'
EOF
    run all all
    expect_success
    expect_stdout <<'EOF'
child
minimake: Nothing to be done for 'all'.
EOF
}

gnu_make_overlap()
{
    command -v make >/dev/null 2>&1 || exit 77
    cat >Makefile <<'EOF'
PREFIX=hello
all: left right
	@printf '%s %s: %s\n' '$(PREFIX)' '$@' '$^'
left: shared
	printf 'left\n'
right: shared
	@printf 'right\n'
shared:
	@printf 'shared\n'
EOF
    MAKEFLAGS= MFLAGS= GNUMAKEFLAGS= MAKEFILES= MAKELEVEL=0 \
        make --no-print-directory -rR -f Makefile >gnu.stdout 2>gnu.stderr
    [ "$?" -eq 0 ] || fail 'GNU make reference command failed'
    run -f Makefile
    expect_success
    cmp -s gnu.stdout "$OUT" || fail 'Behavior differs from GNU make on the shared subset'
}

run_case commands_logged_before_execution
run_case command_whitespace_and_silence
run_case commands_use_separate_shells
run_case shell_builtins_and_external_commands
run_case recipe_failure_stops_all_building
run_case nonexistent_shell_command
run_case shell_terminated_by_signal
run_case dependencies_built_left_to_right_once
run_case repeated_target_not_rebuilt
run_case dependency_then_explicit_target_not_rebuilt
run_case empty_rules_nothing_to_do
run_case existing_target_without_dependencies
run_case older_file_dependency_is_up_to_date
run_case equal_file_dates_are_up_to_date
run_case newer_file_dependency_rebuilds
run_case subsecond_file_dates_are_compared
run_case missing_dependency_file_is_built_first
run_case nothing_to_do_dependency_does_not_force_recipe
run_case rebuilt_virtual_dependency_forces_recipe
run_case empty_parent_after_explicit_child
run_case empty_parent_after_child_build
run_case gnu_make_overlap

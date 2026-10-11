#!/bin/sh

automatic_variable_forms_and_deduplication()
{
    cat >Makefile <<'EOF'
all: left right left
	@printf '%s|%s|%s\n' '$@' '$<' '$^'
	@printf '%s|%s|%s\n' '$(@)' '${<}' '$(^)'
left:
right:
EOF
    run
    expect_success
    expect_stdout <<'EOF'
all|left|left right
all|left|left right
EOF
}

automatic_variables_without_dependencies()
{
    cat >Makefile <<'EOF'
all:
	@printf '%s|%s|%s\n' '$@' '$<' '$^'
EOF
    run
    expect_success
    expect_stdout <<'EOF'
all||
EOF
}

automatic_variables_inside_recursive_variable()
{
    cat >Makefile <<'EOF'
PRINT=@printf '%s|%s|%s\n' '$@' '$<' '$^'
all: source
	$(PRINT)
source:
EOF
    run
    expect_success
    expect_stdout <<'EOF'
all|source|source
EOF
}

phony_target_ignores_existing_file()
{
    cat >Makefile <<'EOF'
.PHONY: clean
clean:
	@printf 'clean\n'
EOF
    : >clean
    run clean
    expect_success
    expect_stdout <<'EOF'
clean
EOF
}

phony_repeated_target_is_nothing_to_do()
{
    cat >Makefile <<'EOF'
all:
	@printf 'all\n'
.PHONY: all
EOF
    : >all
    run all all
    expect_success
    expect_stdout <<'EOF'
all
minimake: Nothing to be done for 'all'.
EOF
}

phony_dependency_forces_parent()
{
    cat >Makefile <<'EOF'
result: force
	@printf 'result\n'
force:
.PHONY: force
EOF
    : >result
    run result
    expect_success
    expect_stdout <<'EOF'
result
EOF
}

phony_declaration_without_separate_rule()
{
    printf '.PHONY: clean\n' >Makefile
    run clean
    expect_success
    expect_stdout <<'EOF'
minimake: Nothing to be done for 'clean'.
EOF
}

phony_shared_dependency_runs_once()
{
    cat >Makefile <<'EOF'
all: left right
	@printf 'all\n'
left: shared
	@printf 'left\n'
right: shared
	@printf 'right\n'
shared:
	@printf 'shared\n'
.PHONY: all left right shared
EOF
    run all shared
    expect_success
    expect_stdout <<'EOF'
shared
left
right
all
minimake: Nothing to be done for 'shared'.
EOF
}

pattern_dependencies_and_automatic_variables()
{
    cat >Makefile <<'EOF'
all: sample.o
	@printf 'all\n'
%.o: %.c header.h
	@printf '%s|%s|%s\n' '$@' '$<' '$^'
EOF
    : >sample.c
    : >header.h
    run
    expect_success
    expect_stdout <<'EOF'
sample.o|sample.c|sample.c header.h
all
EOF
}

explicit_rule_preferred_over_pattern()
{
    cat >Makefile <<'EOF'
all: sample.o
	@printf 'all\n'
%.o:
	@printf 'pattern\n'
sample.o:
	@printf 'explicit\n'
EOF
    run
    expect_success
    expect_stdout <<'EOF'
explicit
all
EOF
}

shortest_pattern_stem_preferred()
{
    cat >Makefile <<'EOF'
all: foo.o
%.o: %.c
	@printf 'generic\n'
f%.o: %.c
	@printf 'specific %s\n' '$<'
EOF
    : >foo.c
    : >oo.c
    run
    expect_success
    expect_stdout <<'EOF'
specific oo.c
EOF
}

equally_short_pattern_stems_keep_definition_order()
{
    cat >Makefile <<'EOF'
all: foo.o
f%.o: %.c
	@printf 'first %s\n' '$<'
%o.o: %.f
	@printf 'second %s\n' '$<'
EOF
    : >oo.c
    : >fo.f
    run
    expect_success
    expect_stdout <<'EOF'
first oo.c
EOF
}

only_first_percent_is_special()
{
    cat >Makefile <<'EOF'
all: name.%.out
%.%.out: %.%.in
	@printf '%s|%s\n' '$@' '$<'
EOF
    : >name.%.in
    run
    expect_success
    expect_stdout <<'EOF'
name.%.out|name.%.in
EOF
}

pattern_chain_and_target_deduplication()
{
    cat >Makefile <<'EOF'
all: one.out two.out one.out
	@printf 'all\n'
%.out: %.mid
	@printf '%s from %s\n' '$@' '$<'
%.mid: %.src
	@printf '%s from %s\n' '$@' '$<'
EOF
    : >one.src
    : >two.src
    run
    expect_success
    expect_stdout <<'EOF'
one.mid from one.src
one.out from one.mid
two.mid from two.src
two.out from two.mid
all
EOF
}

pattern_existing_target_is_up_to_date()
{
    cat >Makefile <<'EOF'
%.o: %.c
	@printf 'should not run\n'
EOF
    : >sample.c
    : >sample.o
    touch -t 200001010000 sample.c
    touch -t 200001010001 sample.o
    run sample.o
    expect_success
    expect_stdout <<'EOF'
minimake: 'sample.o' is up to date.
EOF
}

concrete_pattern_target_containing_percent_is_deduplicated()
{
    cat >Makefile <<'EOF'
%.out:
	@printf '%s\n' '$@'
EOF
    run 'literal%.out' 'literal%.out'
    expect_success
    expect_stdout <<'EOF'
literal%.out
minimake: 'literal%.out' is up to date.
EOF
}

pretty_mode_never_remakes_missing_makefile()
{
    cat >existing.mk <<'EOF'
all:
	@touch should_not_exist
generated.mk:
	@touch generated.mk
EOF
    run -f generated.mk -f existing.mk -p
    expect_error
    expect_empty_stdout
    expect_absent generated.mk
    expect_absent should_not_exist
}

generated_makefile_is_reloaded()
{
    cat >existing.mk <<'EOF'
all: generated_target
	@printf 'all\n'
generated.mk:
	@printf 'generated_target:\n\t@printf "generated target\\n"\n' >generated.mk
EOF
    run all -f generated.mk -f existing.mk
    expect_status 0
    expect_file generated.mk
    expect_stdout <<'EOF'
generated target
all
EOF
    expect_stderr <<'EOF'
minimake: generated.mk: No such file or directory
EOF
}

generated_makefile_changes_default_target()
{
    cat >existing.mk <<'EOF'
all:
	@printf 'old default\n'
generated.mk:
	@printf 'generated_target:\n\t@printf "new default\\n"\n' >generated.mk
EOF
    run -f generated.mk -f existing.mk
    expect_status 0
    expect_stdout <<'EOF'
new default
EOF
    expect_stderr <<'EOF'
minimake: generated.mk: No such file or directory
EOF
}

self_build_and_clean()
{
    expect_file "$PROJECT_ROOT/Minimakefile"
    mkdir src || fail 'Cannot create isolated source directory'
    for source_file in "$PROJECT_ROOT"/src/*.[ch]; do
        cp "$source_file" src/ || fail "Cannot copy $source_file"
    done
    cp "$PROJECT_ROOT/Minimakefile" . || fail 'Cannot copy Minimakefile'
    run -f Minimakefile
    expect_success
    [ -x ./minimake ] || fail 'Minimakefile did not produce a binary'
    printf 'all:\n\t@printf "self built\\n"\n' >smoke.mk
    ./minimake -f smoke.mk >"$OUT" 2>"$ERR"
    STATUS=$?
    expect_success
    expect_stdout <<'EOF'
self built
EOF
    ./minimake -f Minimakefile clean >"$OUT" 2>"$ERR"
    STATUS=$?
    expect_success
    expect_absent minimake
    [ -z "$(find src -type f -name '*.o' -print)" ] \
        || fail 'Self clean left object files behind'
}

run_case automatic_variable_forms_and_deduplication
run_case automatic_variables_without_dependencies
run_case automatic_variables_inside_recursive_variable
run_case phony_target_ignores_existing_file
run_case phony_repeated_target_is_nothing_to_do
run_case phony_dependency_forces_parent
run_case phony_declaration_without_separate_rule
run_case phony_shared_dependency_runs_once
run_case pattern_dependencies_and_automatic_variables
run_case explicit_rule_preferred_over_pattern
run_case shortest_pattern_stem_preferred
run_case equally_short_pattern_stems_keep_definition_order
run_case only_first_percent_is_special
run_case pattern_chain_and_target_deduplication
run_case pattern_existing_target_is_up_to_date
run_case concrete_pattern_target_containing_percent_is_deduplicated
run_case pretty_mode_never_remakes_missing_makefile
run_case generated_makefile_is_reloaded
run_case generated_makefile_changes_default_target
run_case self_build_and_clean

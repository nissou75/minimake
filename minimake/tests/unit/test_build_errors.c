#include <criterion/criterion.h>
#include <criterion/redirect.h>

#include "build.h"
#include "model.h"

Test(build, cycles_report_an_error)
{
    cr_redirect_stderr();
    struct makefile makefile = { 0 };
    struct rule *first = add_rule(&makefile, "cycle-first");
    struct rule *second = add_rule(&makefile, "cycle-second");
    append_string(&first->dependencies, "cycle-second");
    append_string(&second->dependencies, "cycle-first");
    cr_assert_eq(build_target(&makefile, "cycle-first"), 2);
    destroy_makefile(&makefile);
}

#ifndef BUILD_H
#define BUILD_H

#include "model.h"

/* Build once per invocation; report status only for the requested target. */
int build_target(struct makefile *makefile, const char *target);
int build_target_silent(struct makefile *makefile, const char *target);

#endif /* ! BUILD_H */

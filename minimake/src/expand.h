#ifndef EXPAND_H
#define EXPAND_H

#include "model.h"

/* Return an owned string, or NULL after reporting an expansion error. */
char *expand_string(const struct makefile *makefile, const char *input,
                    const struct rule *rule);

#endif /* ! EXPAND_H */

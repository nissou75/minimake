#ifndef PATTERN_H
#define PATTERN_H

#include <stdbool.h>
#include <stddef.h>

#include "model.h"

bool match_pattern(const char *pattern, const char *target, size_t *stem_size);
char *substitute_pattern(const char *pattern, const char *stem);
struct rule *resolve_pattern(struct makefile *makefile, const char *target);

#endif /* ! PATTERN_H */

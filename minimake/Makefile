CC = cc
CPPFLAGS = -D_POSIX_C_SOURCE=200809L -Isrc
CFLAGS = -std=c99 -pedantic -Werror -Wall -Wextra -Wvla
LDFLAGS =
LDLIBS =

SOURCES = $(wildcard src/*.c)
HEADERS = $(wildcard src/*.h)
OBJECTS = $(SOURCES:.c=.o)
CORE_SOURCES = $(filter-out src/main.c,$(SOURCES))
UNIT_SOURCES = $(wildcard tests/unit/*.c)
UNIT_OBJECTS = $(patsubst %.c,build/unit/%.o,$(CORE_SOURCES) $(UNIT_SOURCES))
CRITERION_CFLAGS = $(shell pkg-config --cflags criterion)
CRITERION_LIBS = $(shell pkg-config --libs criterion)
DEPENDENCIES = $(OBJECTS:.o=.d) $(UNIT_OBJECTS:.o=.d)

minimake: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS)

src/%.o: src/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c -o $@ $<

build/unit/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CRITERION_CFLAGS) -MMD -MP -c -o $@ $<

build/tests_unit: $(UNIT_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(UNIT_OBJECTS) $(CRITERION_LIBS) $(LDLIBS)

check: check-unit check-functional

check-unit: build/tests_unit
	./build/tests_unit

check-functional: minimake
	sh tests/tests.sh

build/sanitize/minimake: $(SOURCES) $(HEADERS)
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -g -fno-omit-frame-pointer \
		-fsanitize=address,undefined $(SOURCES) -o $@

check-sanitize: build/sanitize/minimake
	ASAN_OPTIONS=detect_leaks=1 BIN="$(CURDIR)/build/sanitize/minimake" \
		sh tests/tests.sh

format:
	clang-format -i $(SOURCES) $(HEADERS) $(UNIT_SOURCES)

check-format:
	clang-format --dry-run --Werror $(SOURCES) $(HEADERS) $(UNIT_SOURCES)

clean:
	rm -f $(OBJECTS) $(OBJECTS:.o=.d) minimake
	rm -rf build

-include $(DEPENDENCIES)

.PHONY: check check-unit check-functional check-sanitize format check-format clean

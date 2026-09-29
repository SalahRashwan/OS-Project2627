# @File: Makefile
# @Purpose: Builds odysseus, ithaca, and island from the reusable src/
#           modules plus the vendor lib/sphragis.o (island only). The
#           default target builds only the three delivered executables.
#           Test-only targets:
#             make test      style scan + functional/loader-field suite
#                            (rebuilds tests/bin/loader_check first)
#             make faults    allocation/write fault-injection suite
#             make memcheck  Valgrind suite (SIGINT, EOF, failures)
#             make pipes     real broken-pipe suite (SIGPIPE), run once
#                            plainly and once under Valgrind
#             make check     all of the above
#             make package GROUP=<n>  builds G<n>_F1.tar
# @Author: Salah Ahmed Salaheldin Adly Rashwan
# @Date: 2026-09-29

CC = gcc
CPPFLAGS = -D_GNU_SOURCE -Iinclude -Ilib
CFLAGS = -std=gnu11 -Wall -Wextra -g -MMD -MP
LDFLAGS =

BUILD_DIR = build
SRC_DIR = src
LIB_DIR = lib
TEST_DIR = tests
TEST_BIN = $(TEST_DIR)/bin

SPHRAGIS_OBJ = $(LIB_DIR)/sphragis.o

# Shared modules linked into every executable.
COMMON_SRCS = io.c text.c config.c lifecycle.c
# Process-specific modules.
ODYSSEUS_SRCS = Odysseus.c commands.c
ITHACA_SRCS = Ithaca.c voyages.c
ISLAND_SRCS = Island.c routes.c stock.c

COMMON_OBJS = $(addprefix $(BUILD_DIR)/,$(COMMON_SRCS:.c=.o))
ODYSSEUS_OBJS = $(addprefix $(BUILD_DIR)/,$(ODYSSEUS_SRCS:.c=.o)) $(COMMON_OBJS)
ITHACA_OBJS = $(addprefix $(BUILD_DIR)/,$(ITHACA_SRCS:.c=.o)) $(COMMON_OBJS)
ISLAND_OBJS = $(addprefix $(BUILD_DIR)/,$(ISLAND_SRCS:.c=.o)) $(COMMON_OBJS) $(SPHRAGIS_OBJ)

# Loader modules (no entry point) shared by the test-only field checker.
LOADER_OBJS = $(addprefix $(BUILD_DIR)/,voyages.o routes.o stock.o) $(COMMON_OBJS) $(SPHRAGIS_OBJ)

ALL_STUDENT_SRCS = $(COMMON_SRCS) $(ODYSSEUS_SRCS) $(ITHACA_SRCS) $(ISLAND_SRCS)
ALL_DEPS = $(addprefix $(BUILD_DIR)/,$(ALL_STUDENT_SRCS:.c=.d)) \
           $(BUILD_DIR)/loader_check.d $(BUILD_DIR)/fault_inject.d

FAULT_WRAP = -Wl,--wrap=malloc,--wrap=realloc,--wrap=vasprintf,--wrap=write
FAULT_OBJ = $(BUILD_DIR)/fault_inject.o
FAULT_BINS = $(TEST_BIN)/odysseus-fault $(TEST_BIN)/ithaca-fault $(TEST_BIN)/island-fault

.PHONY: all clean test faults memcheck pipes check style package

# Default target: build all three required executables.
all: odysseus ithaca island

odysseus: $(ODYSSEUS_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

ithaca: $(ITHACA_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

# island is the only executable linked against the real Sphragis object.
island: $(ISLAND_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/%.o: $(TEST_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) -I$(TEST_DIR) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR) $(TEST_BIN):
	mkdir -p $@

-include $(ALL_DEPS)

# Test-only field checker, rebuilt whenever any loader source changes.
$(TEST_BIN)/loader_check: $(BUILD_DIR)/loader_check.o $(LOADER_OBJS) | $(TEST_BIN)
	$(CC) $(LDFLAGS) -o $@ $^

# Test-only fault-injection builds of the three real programs.
$(TEST_BIN)/odysseus-fault: $(ODYSSEUS_OBJS) $(FAULT_OBJ) | $(TEST_BIN)
	$(CC) $(LDFLAGS) $(FAULT_WRAP) -o $@ $^

$(TEST_BIN)/ithaca-fault: $(ITHACA_OBJS) $(FAULT_OBJ) | $(TEST_BIN)
	$(CC) $(LDFLAGS) $(FAULT_WRAP) -o $@ $^

$(TEST_BIN)/island-fault: $(ISLAND_OBJS) $(FAULT_OBJ) | $(TEST_BIN)
	$(CC) $(LDFLAGS) $(FAULT_WRAP) -o $@ $^

# Removes generated objects, executables, test binaries, and logs; never
# touches the supplied lib/sphragis.o or any fixture data.
clean:
	rm -f $(BUILD_DIR)/*.o $(BUILD_DIR)/*.d odysseus ithaca island
	rm -rf $(TEST_BIN) $(TEST_DIR)/valgrind-logs $(TEST_DIR)/fault-logs $(TEST_DIR)/pipe-logs

style:
	python3 $(TEST_DIR)/style_check.py

test: all $(TEST_BIN)/loader_check style
	./$(TEST_DIR)/run_functional_tests.sh

faults: $(FAULT_BINS)
	./$(TEST_DIR)/run_fault_tests.sh

memcheck: all
	./$(TEST_DIR)/run_memcheck.sh

pipes: all
	python3 $(TEST_DIR)/pipe_tests.py
	python3 $(TEST_DIR)/pipe_tests.py --valgrind

check: test faults memcheck pipes

# Builds G<GROUP>_F1.tar with a top-level directory of the same name.
# GROUP must be the real group number; it is never guessed.
package: clean
	@if [ -z "$(GROUP)" ]; then echo "Usage: make package GROUP=<group number>"; exit 1; fi
	tar --exclude='./G*_F1.tar' --transform 's,^\.,G$(GROUP)_F1,' -cf G$(GROUP)_F1.tar .
	tar -tf G$(GROUP)_F1.tar

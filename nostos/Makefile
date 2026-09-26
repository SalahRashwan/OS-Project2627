# @File: Makefile
# @Purpose: Builds odysseus, ithaca, and island from the reusable
#           src/ modules plus the vendor lib/sphragis.o (island only).
#           Default target builds all three from a clean directory.
# @Author: Salah Ahmed Salaheldin Adly Rashwan
# @Date: 2026-09-21

CC = gcc
CPPFLAGS = -D_GNU_SOURCE -Iinclude -Ilib
CFLAGS = -std=gnu11 -Wall -Wextra -g -MMD -MP
LDFLAGS =

BUILD_DIR = build
SRC_DIR = src
LIB_DIR = lib

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

ALL_STUDENT_SRCS = $(COMMON_SRCS) $(ODYSSEUS_SRCS) $(ITHACA_SRCS) $(ISLAND_SRCS)
ALL_DEPS = $(addprefix $(BUILD_DIR)/,$(ALL_STUDENT_SRCS:.c=.d))

.PHONY: all clean test memcheck package

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

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

-include $(ALL_DEPS)

# Removes only generated student objects/executables; never touches
# the supplied lib/sphragis.o or any fixture data.
clean:
	rm -f $(BUILD_DIR)/*.o $(BUILD_DIR)/*.d odysseus ithaca island

# Runs the reproducible functional test driver (see tests/).
test: all
	./tests/run_functional_tests.sh

# Runs Valgrind memcheck against each executable with a fixture
# argument set (see tests/ for the exact invocations used).
memcheck: all
	./tests/run_memcheck.sh

# Builds a fresh-extraction-ready tar archive (name left to the
# student; see docs/audit-handoff.md section 9 for the procedure).
package: clean all
	@echo "Build docs/audit-handoff.md section 9 procedure to package."

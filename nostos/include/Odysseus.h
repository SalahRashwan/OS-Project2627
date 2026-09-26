#ifndef NOSTOS_ODYSSEUS_ENTRY_H
#define NOSTOS_ODYSSEUS_ENTRY_H

/*
 * @File: Odysseus.h
 * @Purpose: Entry-point header for ./odysseus <config.dat>. Odysseus is
 *           the only Phase 1 process with an interactive terminal.
 * @Author: Salah Ahmed Salaheldin Adly Rashwan
 * @Date: 2026-09-21
 */

/* System Includes */
#include <errno.h>
#include <poll.h>
#include <stdlib.h>
#include <unistd.h>

/* Project Includes */
#include "types.h"
#include "config.h"
#include "commands.h"
#include "lifecycle.h"
#include "text.h"
#include "io.h"
#include "status.h"

/* Chunk size for one nonblocking-in-spirit (but poll-gated) stdin read. */
#define TERMINAL_READ_CHUNK_SIZE 256

#endif

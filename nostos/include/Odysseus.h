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

/* Chunk size for one poll-gated stdin read. */
#define TERMINAL_READ_CHUNK_SIZE 256
/* Number of descriptors watched by the terminal loop (stdin, signalfd). */
#define TERMINAL_POLL_COUNT 2
/* Prompt printed before every command. */
#define TERMINAL_PROMPT "$ "

/* Fixed stderr diagnostics. They are literals so that reporting an
 * allocation failure never needs another allocation. */
#define ERROR_TERMINAL_WRITE "Error: writing to the terminal failed.\n"
#define ERROR_TERMINAL_READ "Error: reading from the terminal failed.\n"
#define ERROR_TERMINAL_WAIT "Error: waiting for terminal input failed.\n"
#define ERROR_SIGNAL_READ "Error: reading the CTRL+C notification failed.\n"
#define ERROR_INPUT_MEMORY "Error: out of memory while reading a command.\n"
#define ERROR_PARSE_MEMORY "Error: out of memory while parsing a command.\n"

#endif

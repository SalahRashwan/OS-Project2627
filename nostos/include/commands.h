#ifndef NOSTOS_COMMANDS_H
#define NOSTOS_COMMANDS_H

/*
 * @File: commands.h
 * @Purpose: Syntax-only Odysseus command parser (Phase 1). Classifies
 *           one already-dequeued terminal line, validates arity/second
 *           words/numeric fields, and extracts arguments. Never opens
 *           a descriptor, never checks voyage/route/market state, and
 *           never prints anything itself; see Odysseus.c for output.
 * @Author: Salah Ahmed Salaheldin Adly Rashwan
 * @Date: 2026-09-21
 */

/* System Includes */
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/* Project Includes */
#include "types.h"

/* Exact required result lines (P p.15-18; official test sheet T p.2). */
#define COMMAND_OK_MESSAGE "Command OK\n"
#define UNKNOWN_COMMAND_MESSAGE "Unknown command\n"

/* Recommended usage strings (guide section 11.2). ACCEPT/BUY/MAP are
 * fixed by the official test sheet; the others follow the same style
 * for consistency and are not additional golden strings. */
#define USAGE_CONNECT "Usage: CONNECT ITHACA\n"
#define USAGE_LIST_VOYAGES "Usage: LIST VOYAGES\n"
#define USAGE_LIST_MARKET "Usage: LIST MARKET\n"
#define USAGE_ACCEPT "Usage: ACCEPT <voyage_id>\n"
#define USAGE_SAIL "Usage: SAIL <island>\n"
#define USAGE_MAP "Usage: MAP\n"
#define USAGE_BUY "Usage: BUY <product> <amount>\n"
#define USAGE_SELL "Usage: SELL <product> <amount>\n"
#define USAGE_STATUS "Usage: STATUS\n"
#define USAGE_DELIVER "Usage: DELIVER\n"
#define USAGE_CLAIM "Usage: CLAIM\n"

/***********************************************
 * @Name: parseCommand
 * @Def: Classifies and validates one complete terminal line (without
 *       its trailing newline) against the eleven Phase 1 commands.
 * @Arg: In: psLine = the line to parse; not modified.
 *       Out: pstCommand = filled with the recognized kind/arguments on
 *            PARSE_OK; psArg1 is owned and only meaningful on
 *            PARSE_OK, must be released with destroyParsedCommand().
 *       Out: pstUsage = filled with one or two borrowed usage lines
 *            on PARSE_USAGE; untouched otherwise.
 * @Ret: PARSE_OK (valid command), PARSE_UNKNOWN (unrecognized verb),
 *       PARSE_USAGE (recognized verb, invalid syntax), PARSE_EMPTY
 *       (blank line), or PARSE_ERROR (memory allocation failed while
 *       parsing; nothing owned is left in pstCommand).
 ***********************************************/
eParseStatus parseCommand(const char *psLine, tParsedCommand *pstCommand, tUsageMessage *pstUsage);

/***********************************************
 * @Name: destroyParsedCommand
 * @Def: Frees the owned argument copy of a parsed command, if any.
 * @Arg: In/Out: pstCommand = command to release.
 * @Ret: None.
 ***********************************************/
void destroyParsedCommand(tParsedCommand *pstCommand);

#endif

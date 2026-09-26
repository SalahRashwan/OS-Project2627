#ifndef NOSTOS_ITHACA_ENTRY_H
#define NOSTOS_ITHACA_ENTRY_H

/*
 * @File: Ithaca.h
 * @Purpose: Entry-point header for ./ithaca <config.dat> <voyages.dat>.
 *           Pulls in every system and project header Ithaca.c needs,
 *           so the source file itself includes only this header, per
 *           the style guide's "own includes only" convention.
 * @Author: Salah Ahmed Salaheldin Adly Rashwan
 * @Date: 2026-09-21
 */

/* System Includes */
#include <unistd.h>

/* Project Includes */
#include "types.h"
#include "config.h"
#include "voyages.h"
#include "lifecycle.h"
#include "io.h"
#include "status.h"

/* Fixed stderr diagnostics (literals: reporting needs no allocation). */
#define ERROR_ITHACA_WRITE "Error: Ithaca could not write to standard output.\n"
#define ERROR_ITHACA_SIGNAL "Error: Ithaca failed while waiting for CTRL+C.\n"

#endif

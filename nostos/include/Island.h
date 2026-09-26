#ifndef NOSTOS_ISLAND_ENTRY_H
#define NOSTOS_ISLAND_ENTRY_H

/*
 * @File: Island.h
 * @Purpose: Entry-point header for ./island <config.dat> <stock.db>.
 *           One executable represents any island by configuration.
 * @Author: Salah Ahmed Salaheldin Adly Rashwan
 * @Date: 2026-09-21
 */

/* System Includes */
#include <unistd.h>

/* Project Includes */
#include "types.h"
#include "config.h"
#include "routes.h"
#include "stock.h"
#include "lifecycle.h"
#include "io.h"
#include "status.h"

/* Fixed stderr diagnostics (literals: reporting needs no allocation). */
#define ERROR_ISLAND_WRITE "Error: Island could not write to standard output.\n"
#define ERROR_ISLAND_SIGNAL "Error: Island failed while waiting for CTRL+C.\n"

#endif

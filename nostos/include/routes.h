#ifndef NOSTOS_ROUTES_H
#define NOSTOS_ROUTES_H

/*
 * @File: routes.h
 * @Purpose: The Sphragis ownership adapter: turns a raw, unvalidated
 *           route list into the real library-validated valid route
 *           list, preserving IP/port endpoints the library itself does
 *           not track. See docs/design.md section 4 for the full
 *           ownership walkthrough.
 * @Author: Salah Ahmed Salaheldin Adly Rashwan
 * @Date: 2026-09-21
 */

/* System Includes */
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/* Project Includes */
#include "types.h"

/***********************************************
 * @Name: filterIslandRoutes
 * @Def: Validates an island's raw candidate routes using the real
 *       SPHRAGIS_filter_island_configuration(), then rebuilds a fresh,
 *       independently owned list of exactly the surviving
 *       destinations, each with its original IP/port endpoint
 *       restored. The raw list is left untouched by this call; the
 *       caller destroys it separately once filtering succeeds.
 * @Arg: In: psIslandName = the island whose configuration is being
 *           validated (borrowed; not freed by this function).
 *       In: pstRawRoutes = unvalidated candidate routes, read exactly
 *           as parsed from island.dat; never mutated here.
 *       Out: pstValidRoutes = filled with the surviving routes on
 *            success; left empty and safe to destroy on failure.
 * @Ret: The new nonnegative valid route count on success (matches
 *       pstValidRoutes->nCount), or a negative SPHRAGIS_ERROR_* /
 *       NOSTOS_ERROR code on failure.
 ***********************************************/
int filterIslandRoutes(const char *psIslandName, const tRouteList *pstRawRoutes, tRouteList *pstValidRoutes);

#endif

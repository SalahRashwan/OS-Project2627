#ifndef NOSTOS_CONFIG_H
#define NOSTOS_CONFIG_H

/*
 * @File: config.h
 * @Purpose: Loaders for the three Nostos Phase 1 configuration formats
 *           (odysseus.dat, ithaca.dat, island.dat) plus the shared
 *           destructors for the aggregates they build. Island route
 *           validation itself lives in routes.h; this file only parses
 *           the raw, unvalidated candidate route list.
 * @Author: Salah Ahmed Salaheldin Adly Rashwan
 * @Date: 2026-09-21
 */

/* Own */
#include "types.h"

/* Exact marker line (after CRLF normalization) that introduces an
 * island's routes section. */
#define ROUTES_SECTION_MARKER "--- ROUTES ---"

/* Defensive bounds applied to any "<IP> <PORT>" configuration line. */
#define MIN_PORT_NUMBER 1
#define MAX_PORT_NUMBER 65535

/***********************************************
 * @Name: loadOdysseusConfig
 * @Def: Opens, parses, and closes an Odysseus configuration file.
 * @Arg: In: psPath = path to the configuration file.
 *       Out: pstConfig = filled on success; left in a safe,
 *            destroyable state on failure (no leaked partial entries).
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR otherwise.
 ***********************************************/
int loadOdysseusConfig(const char *psPath, tOdysseusConfig *pstConfig);

/***********************************************
 * @Name: destroyOdysseusConfig
 * @Def: Frees every owned allocation inside an Odysseus configuration.
 * @Arg: In/Out: pstConfig = configuration to release; safe to call on
 *       a zeroed or partially filled instance.
 * @Ret: None.
 ***********************************************/
void destroyOdysseusConfig(tOdysseusConfig *pstConfig);

/***********************************************
 * @Name: loadIthacaConfig
 * @Def: Opens, parses, and closes an Ithaca configuration file.
 * @Arg: In: psPath = path to the configuration file.
 *       Out: pstConfig = filled on success; safe to destroy on failure.
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR otherwise.
 ***********************************************/
int loadIthacaConfig(const char *psPath, tIthacaConfig *pstConfig);

/***********************************************
 * @Name: destroyIthacaConfig
 * @Def: Frees every owned allocation inside an Ithaca configuration.
 * @Arg: In/Out: pstConfig = configuration to release.
 * @Ret: None.
 ***********************************************/
void destroyIthacaConfig(tIthacaConfig *pstConfig);

/***********************************************
 * @Name: loadIslandConfig
 * @Def: Opens, parses, and closes an island configuration file,
 *       including its raw (not yet Sphragis-validated) routes section.
 * @Arg: In: psPath = path to the configuration file.
 *       Out: pstConfig = filled with name/folder/endpoint/capacity;
 *            its stRoutes list is left empty (the valid list is built
 *            later by routes.c after filtering).
 *       Out: pstRawRoutes = filled with every candidate route in file
 *            order; owned by the caller and independent of pstConfig.
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR otherwise.
 ***********************************************/
int loadIslandConfig(const char *psPath, tIslandConfig *pstConfig, tRouteList *pstRawRoutes);

/***********************************************
 * @Name: destroyIslandConfig
 * @Def: Frees every owned allocation inside an island configuration,
 *       including its (post-filtering) valid route list.
 * @Arg: In/Out: pstConfig = configuration to release.
 * @Ret: None.
 ***********************************************/
void destroyIslandConfig(tIslandConfig *pstConfig);

/***********************************************
 * @Name: initRouteList
 * @Def: Resets a route list to a safe, destroyable empty state. Used
 *       for both the raw candidate list and the post-filter valid list.
 * @Arg: Out: pstList = list to initialize.
 * @Ret: None.
 ***********************************************/
void initRouteList(tRouteList *pstList);

/***********************************************
 * @Name: routeListAppend
 * @Def: Appends one already-owned route to a growable route list,
 *       moving ownership of its strings into the list on success.
 *       Shared by the raw-route loader and the Sphragis adapter.
 * @Arg: In/Out: pstList = list to grow.
 *       In: psDestination, psIp, nPort = route fields; the two
 *           strings are moved (not copied) into the list on success.
 * @Ret: NOSTOS_OK on success (ownership moved); NOSTOS_ERROR on
 *       allocation failure (caller still owns and must free the
 *       strings).
 ***********************************************/
int routeListAppend(tRouteList *pstList, char *psDestination, char *psIp, int nPort);

/***********************************************
 * @Name: destroyRouteList
 * @Def: Frees every owned name/IP in a route list and the array
 *       itself. Used for both the raw candidate list and the valid
 *       (post-Sphragis) list.
 * @Arg: In/Out: pstList = route list to release.
 * @Ret: None.
 ***********************************************/
void destroyRouteList(tRouteList *pstList);

#endif

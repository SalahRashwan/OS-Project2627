/*
 * @File: routes.c
 * @Purpose: The Sphragis ownership adapter. Builds a throwaway array of
 *           separately heap-allocated name copies for the library to
 *           mutate (it frees rejected names and preserves survivors),
 *           then rebuilds an independently owned valid route list with
 *           each survivor's original IP/port restored. Never guesses,
 *           hardcodes, or bypasses the real library's decision.
 * @Author: Salah Ahmed Salaheldin Adly Rashwan
 * @Date: 2026-09-21
 */

/* System */
#include <stdlib.h>
#include <string.h>

/* Own */
#include "routes.h"
#include "config.h"
#include "text.h"
#include "status.h"
#include "sphragis.h"

/***********************************************
 * @Name: freeTempNames
 * @Def: Frees every non-NULL slot of a temporary name array and the
 *       array itself. Safe to call with a NULL array (zero-route case).
 * @Arg: In/Out: appsNames = array to release.
 *       In: nCount = number of slots.
 * @Ret: None.
 ***********************************************/
static void freeTempNames(char **appsNames, int nCount) {
    int nIndex = 0;

    if (NULL == appsNames) {
        return;
    }
    for (nIndex = 0; nIndex < nCount; nIndex++) {
        free(appsNames[nIndex]);
    }
    free(appsNames);
}

/***********************************************
 * @Name: buildTempNames
 * @Def: Allocates one separately heap-allocated copy of each raw
 *       route's destination name, as SPHRAGIS_filter_island_configuration
 *       requires it may free them.
 * @Arg: In: pstRawRoutes = source raw routes.
 *       Out: pappsTempNames = set to a newly owned array (or NULL for
 *            zero raw routes) on success.
 * @Ret: NOSTOS_OK / NOSTOS_ERROR.
 ***********************************************/
static int buildTempNames(const tRouteList *pstRawRoutes, char ***pappsTempNames) {
    char **appsTemp = NULL;
    int nIndex = 0;

    if (0 == pstRawRoutes->nCount) {
        *pappsTempNames = NULL;
        return NOSTOS_OK;
    }
    appsTemp = malloc((size_t) pstRawRoutes->nCount * sizeof(char *));
    if (NULL == appsTemp) {
        return NOSTOS_ERROR;
    }
    for (nIndex = 0; nIndex < pstRawRoutes->nCount; nIndex++) {
        appsTemp[nIndex] = NULL;
    }
    for (nIndex = 0; nIndex < pstRawRoutes->nCount; nIndex++) {
        appsTemp[nIndex] = duplicateString(pstRawRoutes->pstRoutes[nIndex].psDestination);
        if (NULL == appsTemp[nIndex]) {
            freeTempNames(appsTemp, pstRawRoutes->nCount);
            return NOSTOS_ERROR;
        }
    }
    *pappsTempNames = appsTemp;
    return NOSTOS_OK;
}

/***********************************************
 * @Name: findRawRouteByName
 * @Def: Case-insensitively finds a raw route by destination name, to
 *       recover the IP/port endpoint the library itself does not
 *       track.
 * @Arg: In: pstRawRoutes, psName.
 * @Ret: A borrowed pointer to the matching raw route, or NULL.
 ***********************************************/
static const tRoute *findRawRouteByName(const tRouteList *pstRawRoutes, const char *psName) {
    int nIndex = 0;

    for (nIndex = 0; nIndex < pstRawRoutes->nCount; nIndex++) {
        if (0 == strcasecmp(pstRawRoutes->pstRoutes[nIndex].psDestination, psName)) {
            return &pstRawRoutes->pstRoutes[nIndex];
        }
    }
    return NULL;
}

/***********************************************
 * @Name: collectSurvivors
 * @Def: Walks the (library-mutated) temporary name array; every
 *       remaining non-NULL slot is a survivor whose owned string is
 *       moved (not copied) into a freshly built valid route, paired
 *       with the IP/port recovered from the matching raw route. An
 *       unmatched survivor is an internal consistency error: no
 *       endpoint is ever fabricated.
 * @Arg: In: pstRawRoutes = source of IP/port endpoints.
 *       In/Out: appsTempNames = survivor names; moved slots become
 *            NULL so the caller's final cleanup pass never double-frees.
 *       In: nRawCount = number of slots in appsTempNames.
 *       Out: pstValidRoutes = grown with each matched survivor.
 * @Ret: NOSTOS_OK / NOSTOS_ERROR.
 ***********************************************/
static int collectSurvivors(const tRouteList *pstRawRoutes, char **appsTempNames, int nRawCount, tRouteList *pstValidRoutes) {
    int nIndex = 0;
    const tRoute *pstMatch = NULL;
    char *psIpCopy = NULL;

    for (nIndex = 0; nIndex < nRawCount; nIndex++) {
        if (NULL == appsTempNames[nIndex]) {
            continue;
        }
        pstMatch = findRawRouteByName(pstRawRoutes, appsTempNames[nIndex]);
        if (NULL == pstMatch) {
            return NOSTOS_ERROR;
        }
        psIpCopy = duplicateString(pstMatch->psIp);
        if (NULL == psIpCopy) {
            return NOSTOS_ERROR;
        }
        if (NOSTOS_OK != routeListAppend(pstValidRoutes, appsTempNames[nIndex], psIpCopy, pstMatch->nPort)) {
            free(psIpCopy);
            return NOSTOS_ERROR;
        }
        appsTempNames[nIndex] = NULL;
    }
    return NOSTOS_OK;
}

/***********************************************
 * @Name: filterIslandRoutes
 * @Def: See routes.h.
 ***********************************************/
int filterIslandRoutes(const char *psIslandName, const tRouteList *pstRawRoutes, tRouteList *pstValidRoutes) {
    char **appsTempNames = NULL;
    SPHRAGIS_Island stIsland;
    int nRawCount = pstRawRoutes->nCount;
    int nLibraryResult = 0;

    initRouteList(pstValidRoutes);
    if (NOSTOS_OK != buildTempNames(pstRawRoutes, &appsTempNames)) {
        return NOSTOS_ERROR;
    }
    stIsland.name = (char *) psIslandName;
    stIsland.known_islands = appsTempNames;
    stIsland.known_island_count = nRawCount;
    nLibraryResult = SPHRAGIS_filter_island_configuration(&stIsland);
    if (0 > nLibraryResult) {
        /* Assumption A19: the header does not document whether the
         * array is mutated on an error return, so free defensively. */
        freeTempNames(appsTempNames, nRawCount);
        return nLibraryResult;
    }
    if (NOSTOS_OK != collectSurvivors(pstRawRoutes, appsTempNames, nRawCount, pstValidRoutes)) {
        freeTempNames(appsTempNames, nRawCount);
        destroyRouteList(pstValidRoutes);
        return NOSTOS_ERROR;
    }
    freeTempNames(appsTempNames, nRawCount);
    return nLibraryResult;
}

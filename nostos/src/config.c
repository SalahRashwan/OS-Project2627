/*
 * @File: config.c
 * @Purpose: Loaders for the odysseus.dat, ithaca.dat, and island.dat
 *           configuration formats, plus the destructors for the
 *           aggregates they build. Island route validation lives in
 *           routes.c; this file only parses the raw candidate routes.
 * @Author: Salah Ahmed Salaheldin Adly Rashwan
 * @Date: 2026-09-21
 */

/* System */
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Own */
#include "config.h"
#include "text.h"
#include "io.h"
#include "status.h"

/***********************************************
 * @Name: readOwnedLine
 * @Def: Reads the next line of a configuration file, treating both a
 *       premature EOF and a real read/allocation error as failure,
 *       since every field below is mandatory in a well-formed file.
 * @Arg: In: nFd, In/Out: pstBuffer, Out: ppsLine.
 * @Ret: NOSTOS_OK / NOSTOS_ERROR.
 ***********************************************/
static int readOwnedLine(int nFd, tLineBuffer *pstBuffer, char **ppsLine) {
    int nResult = readNextLine(nFd, pstBuffer, ppsLine);

    if (LINE_FOUND == nResult) {
        return NOSTOS_OK;
    }
    return NOSTOS_ERROR;
}

/***********************************************
 * @Name: parseEndpointLine
 * @Def: Parses a two-token "<IP> <PORT>" line into an owned endpoint.
 * @Arg: In/Out: psLine = mutable line to tokenize.
 *       Out: pstEndpoint = filled on success.
 * @Ret: NOSTOS_OK / NOSTOS_ERROR.
 ***********************************************/
static int parseEndpointLine(char *psLine, tEndpoint *pstEndpoint) {
    char **appsTokens = NULL;
    int nTokenCount = 0;
    long lPort = 0;

    if (NOSTOS_OK != tokenizeLine(psLine, &appsTokens, &nTokenCount)) {
        return NOSTOS_ERROR;
    }
    if (2 != nTokenCount || NOSTOS_OK != parseDigitsToLong(appsTokens[1], &lPort)) {
        free(appsTokens);
        return NOSTOS_ERROR;
    }
    if (lPort < MIN_PORT_NUMBER || lPort > MAX_PORT_NUMBER) {
        free(appsTokens);
        return NOSTOS_ERROR;
    }
    pstEndpoint->psIp = duplicateString(appsTokens[0]);
    pstEndpoint->nPort = (int) lPort;
    free(appsTokens);
    if (NULL == pstEndpoint->psIp) {
        return NOSTOS_ERROR;
    }
    return NOSTOS_OK;
}

/***********************************************
 * @Name: parseSingleIntLine
 * @Def: Parses a one-token digit-only line bounded to [lMin, lMax].
 * @Arg: In/Out: psLine = mutable line to tokenize.
 *       In: lMin, lMax = inclusive bounds.
 *       Out: pnValue = filled on success.
 * @Ret: NOSTOS_OK / NOSTOS_ERROR.
 ***********************************************/
static int parseSingleIntLine(char *psLine, long lMin, long lMax, int *pnValue) {
    char **appsTokens = NULL;
    int nTokenCount = 0;
    long lValue = 0;

    if (NOSTOS_OK != tokenizeLine(psLine, &appsTokens, &nTokenCount)) {
        return NOSTOS_ERROR;
    }
    if (1 != nTokenCount || NOSTOS_OK != parseDigitsToLong(appsTokens[0], &lValue)) {
        free(appsTokens);
        return NOSTOS_ERROR;
    }
    free(appsTokens);
    if (lValue < lMin || lValue > lMax) {
        return NOSTOS_ERROR;
    }
    *pnValue = (int) lValue;
    return NOSTOS_OK;
}

/* ---- Odysseus configuration ---------------------------------------- */

/***********************************************
 * @Name: initOdysseusConfig
 * @Def: Resets an Odysseus configuration to a safe, destroyable state.
 * @Arg: Out: pstConfig.
 * @Ret: None.
 ***********************************************/
static void initOdysseusConfig(tOdysseusConfig *pstConfig) {
    pstConfig->psName = NULL;
    pstConfig->psStorageFolder = NULL;
    pstConfig->stIthaca.psIp = NULL;
    pstConfig->stIthaca.nPort = 0;
    pstConfig->psInitialIsland = NULL;
    pstConfig->stInitialIslandEndpoint.psIp = NULL;
    pstConfig->stInitialIslandEndpoint.nPort = 0;
    pstConfig->nGold = 0;
    pstConfig->nFoodCount = 0;
    pstConfig->pstFoods = NULL;
}

/***********************************************
 * @Name: destroyOdysseusConfig
 * @Def: See config.h.
 ***********************************************/
void destroyOdysseusConfig(tOdysseusConfig *pstConfig) {
    int nIndex = 0;

    free(pstConfig->psName);
    free(pstConfig->psStorageFolder);
    free(pstConfig->stIthaca.psIp);
    free(pstConfig->psInitialIsland);
    free(pstConfig->stInitialIslandEndpoint.psIp);
    if (NULL != pstConfig->pstFoods) {
        for (nIndex = 0; nIndex < pstConfig->nFoodCount; nIndex++) {
            free(pstConfig->pstFoods[nIndex].psProduct);
        }
        free(pstConfig->pstFoods);
    }
    initOdysseusConfig(pstConfig);
}

/***********************************************
 * @Name: parseInitialIslandLine
 * @Def: Parses the three-token "<NAME> <IP> <PORT>" initial-island line.
 * @Arg: In/Out: psLine = mutable line to tokenize.
 *       Out: ppsName, pstEndpoint = filled on success.
 * @Ret: NOSTOS_OK / NOSTOS_ERROR.
 ***********************************************/
static int parseInitialIslandLine(char *psLine, char **ppsName, tEndpoint *pstEndpoint) {
    char **appsTokens = NULL;
    int nTokenCount = 0;
    long lPort = 0;

    if (NOSTOS_OK != tokenizeLine(psLine, &appsTokens, &nTokenCount)) {
        return NOSTOS_ERROR;
    }
    if (3 != nTokenCount || NOSTOS_OK != parseDigitsToLong(appsTokens[2], &lPort)) {
        free(appsTokens);
        return NOSTOS_ERROR;
    }
    if (lPort < MIN_PORT_NUMBER || lPort > MAX_PORT_NUMBER) {
        free(appsTokens);
        return NOSTOS_ERROR;
    }
    *ppsName = duplicateString(appsTokens[0]);
    pstEndpoint->psIp = duplicateString(appsTokens[1]);
    pstEndpoint->nPort = (int) lPort;
    free(appsTokens);
    if (NULL == *ppsName || NULL == pstEndpoint->psIp) {
        return NOSTOS_ERROR;
    }
    return NOSTOS_OK;
}

/***********************************************
 * @Name: parseFoodLine
 * @Def: Parses a two-token "<PRODUCT> <AMOUNT>" food-supply line.
 * @Arg: In/Out: psLine = mutable line to tokenize.
 *       Out: pstFood = filled on success.
 * @Ret: NOSTOS_OK / NOSTOS_ERROR.
 ***********************************************/
static int parseFoodLine(char *psLine, tFoodEntry *pstFood) {
    char **appsTokens = NULL;
    int nTokenCount = 0;
    long lAmount = 0;

    if (NOSTOS_OK != tokenizeLine(psLine, &appsTokens, &nTokenCount)) {
        return NOSTOS_ERROR;
    }
    if (2 != nTokenCount || NOSTOS_OK != parseDigitsToLong(appsTokens[1], &lAmount) || lAmount > INT_MAX) {
        free(appsTokens);
        return NOSTOS_ERROR;
    }
    pstFood->psProduct = duplicateString(appsTokens[0]);
    pstFood->nAmountKg = (int) lAmount;
    free(appsTokens);
    if (NULL == pstFood->psProduct) {
        return NOSTOS_ERROR;
    }
    return NOSTOS_OK;
}

/***********************************************
 * @Name: loadOdysseusIdentity
 * @Def: Reads name, storage folder, and Ithaca endpoint, in that order.
 * @Arg: In: nFd, In/Out: pstBuffer, Out: pstConfig.
 * @Ret: NOSTOS_OK / NOSTOS_ERROR.
 ***********************************************/
static int loadOdysseusIdentity(int nFd, tLineBuffer *pstBuffer, tOdysseusConfig *pstConfig) {
    char *psLine = NULL;

    if (NOSTOS_OK != readOwnedLine(nFd, pstBuffer, &psLine)) {
        return NOSTOS_ERROR;
    }
    pstConfig->psName = duplicateString(psLine);
    free(psLine);
    if (NULL == pstConfig->psName) {
        return NOSTOS_ERROR;
    }
    if (NOSTOS_OK != readOwnedLine(nFd, pstBuffer, &psLine)) {
        return NOSTOS_ERROR;
    }
    pstConfig->psStorageFolder = duplicateString(psLine);
    free(psLine);
    if (NULL == pstConfig->psStorageFolder) {
        return NOSTOS_ERROR;
    }
    if (NOSTOS_OK != readOwnedLine(nFd, pstBuffer, &psLine)) {
        return NOSTOS_ERROR;
    }
    if (NOSTOS_OK != parseEndpointLine(psLine, &pstConfig->stIthaca)) {
        free(psLine);
        return NOSTOS_ERROR;
    }
    free(psLine);
    return NOSTOS_OK;
}

/***********************************************
 * @Name: loadOdysseusResources
 * @Def: Reads the initial island, gold, and the declared food-entry
 *       count, in that order.
 * @Arg: In: nFd, In/Out: pstBuffer, Out: pstConfig, pnFoodCount.
 * @Ret: NOSTOS_OK / NOSTOS_ERROR.
 ***********************************************/
static int loadOdysseusResources(int nFd, tLineBuffer *pstBuffer, tOdysseusConfig *pstConfig, int *pnFoodCount) {
    char *psLine = NULL;

    if (NOSTOS_OK != readOwnedLine(nFd, pstBuffer, &psLine)) {
        return NOSTOS_ERROR;
    }
    if (NOSTOS_OK != parseInitialIslandLine(psLine, &pstConfig->psInitialIsland, &pstConfig->stInitialIslandEndpoint)) {
        free(psLine);
        return NOSTOS_ERROR;
    }
    free(psLine);
    if (NOSTOS_OK != readOwnedLine(nFd, pstBuffer, &psLine)) {
        return NOSTOS_ERROR;
    }
    if (NOSTOS_OK != parseSingleIntLine(psLine, 0, INT_MAX, &pstConfig->nGold)) {
        free(psLine);
        return NOSTOS_ERROR;
    }
    free(psLine);
    if (NOSTOS_OK != readOwnedLine(nFd, pstBuffer, &psLine)) {
        return NOSTOS_ERROR;
    }
    if (NOSTOS_OK != parseSingleIntLine(psLine, 0, INT_MAX, pnFoodCount)) {
        free(psLine);
        return NOSTOS_ERROR;
    }
    free(psLine);
    return NOSTOS_OK;
}

/***********************************************
 * @Name: loadOdysseusFoods
 * @Def: Allocates and fills exactly nFoodCount food entries.
 * @Arg: In: nFd, In/Out: pstBuffer, Out: pstConfig, In: nFoodCount.
 * @Ret: NOSTOS_OK / NOSTOS_ERROR.
 ***********************************************/
static int loadOdysseusFoods(int nFd, tLineBuffer *pstBuffer, tOdysseusConfig *pstConfig, int nFoodCount) {
    char *psLine = NULL;
    int nIndex = 0;

    if (0 == nFoodCount) {
        return NOSTOS_OK;
    }
    pstConfig->pstFoods = malloc((size_t) nFoodCount * sizeof(tFoodEntry));
    if (NULL == pstConfig->pstFoods) {
        return NOSTOS_ERROR;
    }
    for (nIndex = 0; nIndex < nFoodCount; nIndex++) {
        if (NOSTOS_OK != readOwnedLine(nFd, pstBuffer, &psLine)) {
            return NOSTOS_ERROR;
        }
        if (NOSTOS_OK != parseFoodLine(psLine, &pstConfig->pstFoods[nIndex])) {
            free(psLine);
            return NOSTOS_ERROR;
        }
        free(psLine);
        pstConfig->nFoodCount = nIndex + 1;
    }
    return NOSTOS_OK;
}

/***********************************************
 * @Name: failOdysseus
 * @Def: Single cleanup path for a failed Odysseus load.
 * @Arg: In: nFd, In/Out: pstBuffer, pstConfig.
 * @Ret: NOSTOS_ERROR, always.
 ***********************************************/
static int failOdysseus(int nFd, tLineBuffer *pstBuffer, tOdysseusConfig *pstConfig) {
    close(nFd);
    lineBufferDestroy(pstBuffer);
    destroyOdysseusConfig(pstConfig);
    return NOSTOS_ERROR;
}

/***********************************************
 * @Name: loadOdysseusConfig
 * @Def: See config.h.
 ***********************************************/
int loadOdysseusConfig(const char *psPath, tOdysseusConfig *pstConfig) {
    int nFd = -1;
    tLineBuffer stBuffer;
    int nFoodCount = 0;

    initOdysseusConfig(pstConfig);
    lineBufferInit(&stBuffer);
    nFd = safeOpenReadOnly(psPath);
    if (-1 == nFd) {
        return NOSTOS_ERROR;
    }
    if (NOSTOS_OK != loadOdysseusIdentity(nFd, &stBuffer, pstConfig)) {
        return failOdysseus(nFd, &stBuffer, pstConfig);
    }
    if (NOSTOS_OK != loadOdysseusResources(nFd, &stBuffer, pstConfig, &nFoodCount)) {
        return failOdysseus(nFd, &stBuffer, pstConfig);
    }
    if (NOSTOS_OK != loadOdysseusFoods(nFd, &stBuffer, pstConfig, nFoodCount)) {
        return failOdysseus(nFd, &stBuffer, pstConfig);
    }
    close(nFd);
    lineBufferDestroy(&stBuffer);
    return NOSTOS_OK;
}

/* ---- Ithaca configuration -------------------------------------------- */

/***********************************************
 * @Name: initIthacaConfig
 * @Def: Resets an Ithaca configuration to a safe, destroyable state.
 * @Arg: Out: pstConfig.
 * @Ret: None.
 ***********************************************/
static void initIthacaConfig(tIthacaConfig *pstConfig) {
    pstConfig->psName = NULL;
    pstConfig->psMissionFolder = NULL;
    pstConfig->stListen.psIp = NULL;
    pstConfig->stListen.nPort = 0;
}

/***********************************************
 * @Name: destroyIthacaConfig
 * @Def: See config.h.
 ***********************************************/
void destroyIthacaConfig(tIthacaConfig *pstConfig) {
    free(pstConfig->psName);
    free(pstConfig->psMissionFolder);
    free(pstConfig->stListen.psIp);
    initIthacaConfig(pstConfig);
}

/***********************************************
 * @Name: failIthaca
 * @Def: Single cleanup path for a failed Ithaca load.
 * @Arg: In: nFd, In/Out: pstBuffer, pstConfig.
 * @Ret: NOSTOS_ERROR, always.
 ***********************************************/
static int failIthaca(int nFd, tLineBuffer *pstBuffer, tIthacaConfig *pstConfig) {
    close(nFd);
    lineBufferDestroy(pstBuffer);
    destroyIthacaConfig(pstConfig);
    return NOSTOS_ERROR;
}

/***********************************************
 * @Name: loadIthacaConfig
 * @Def: See config.h.
 ***********************************************/
int loadIthacaConfig(const char *psPath, tIthacaConfig *pstConfig) {
    int nFd = -1;
    tLineBuffer stBuffer;
    char *psLine = NULL;

    initIthacaConfig(pstConfig);
    lineBufferInit(&stBuffer);
    nFd = safeOpenReadOnly(psPath);
    if (-1 == nFd) {
        return NOSTOS_ERROR;
    }
    if (NOSTOS_OK != readOwnedLine(nFd, &stBuffer, &psLine)) {
        return failIthaca(nFd, &stBuffer, pstConfig);
    }
    pstConfig->psName = duplicateString(psLine);
    free(psLine);
    if (NULL == pstConfig->psName) {
        return failIthaca(nFd, &stBuffer, pstConfig);
    }
    if (NOSTOS_OK != readOwnedLine(nFd, &stBuffer, &psLine)) {
        return failIthaca(nFd, &stBuffer, pstConfig);
    }
    pstConfig->psMissionFolder = duplicateString(psLine);
    free(psLine);
    if (NULL == pstConfig->psMissionFolder) {
        return failIthaca(nFd, &stBuffer, pstConfig);
    }
    if (NOSTOS_OK != readOwnedLine(nFd, &stBuffer, &psLine)) {
        return failIthaca(nFd, &stBuffer, pstConfig);
    }
    if (NOSTOS_OK != parseEndpointLine(psLine, &pstConfig->stListen)) {
        free(psLine);
        return failIthaca(nFd, &stBuffer, pstConfig);
    }
    free(psLine);
    close(nFd);
    lineBufferDestroy(&stBuffer);
    return NOSTOS_OK;
}

/* ---- Island configuration and raw (unvalidated) routes --------------- */

/***********************************************
 * @Name: initIslandConfig
 * @Def: Resets an island configuration to a safe, destroyable state.
 * @Arg: Out: pstConfig.
 * @Ret: None.
 ***********************************************/
static void initIslandConfig(tIslandConfig *pstConfig) {
    pstConfig->psName = NULL;
    pstConfig->psStorageFolder = NULL;
    pstConfig->stListen.psIp = NULL;
    pstConfig->stListen.nPort = 0;
    pstConfig->nCapacity = 0;
    pstConfig->stRoutes.pstRoutes = NULL;
    pstConfig->stRoutes.nCount = 0;
    pstConfig->stRoutes.nCapacity = 0;
}

/***********************************************
 * @Name: initRouteList
 * @Def: See config.h.
 ***********************************************/
void initRouteList(tRouteList *pstList) {
    pstList->pstRoutes = NULL;
    pstList->nCount = 0;
    pstList->nCapacity = 0;
}

/***********************************************
 * @Name: destroyRouteList
 * @Def: See config.h.
 ***********************************************/
void destroyRouteList(tRouteList *pstList) {
    int nIndex = 0;

    if (NULL != pstList->pstRoutes) {
        for (nIndex = 0; nIndex < pstList->nCount; nIndex++) {
            free(pstList->pstRoutes[nIndex].psDestination);
            free(pstList->pstRoutes[nIndex].psIp);
        }
        free(pstList->pstRoutes);
    }
    initRouteList(pstList);
}

/***********************************************
 * @Name: destroyIslandConfig
 * @Def: See config.h.
 ***********************************************/
void destroyIslandConfig(tIslandConfig *pstConfig) {
    free(pstConfig->psName);
    free(pstConfig->psStorageFolder);
    free(pstConfig->stListen.psIp);
    destroyRouteList(&pstConfig->stRoutes);
    initIslandConfig(pstConfig);
}

/***********************************************
 * @Name: loadIslandHeader
 * @Def: Reads name, storage folder, listen endpoint, port capacity,
 *       and the exact routes-section marker, in that order.
 * @Arg: In: nFd, In/Out: pstBuffer, Out: pstConfig.
 * @Ret: NOSTOS_OK / NOSTOS_ERROR.
 ***********************************************/
static int loadIslandHeader(int nFd, tLineBuffer *pstBuffer, tIslandConfig *pstConfig) {
    char *psLine = NULL;

    if (NOSTOS_OK != readOwnedLine(nFd, pstBuffer, &psLine)) {
        return NOSTOS_ERROR;
    }
    pstConfig->psName = duplicateString(psLine);
    free(psLine);
    if (NULL == pstConfig->psName) {
        return NOSTOS_ERROR;
    }
    if (NOSTOS_OK != readOwnedLine(nFd, pstBuffer, &psLine)) {
        return NOSTOS_ERROR;
    }
    pstConfig->psStorageFolder = duplicateString(psLine);
    free(psLine);
    if (NULL == pstConfig->psStorageFolder) {
        return NOSTOS_ERROR;
    }
    if (NOSTOS_OK != readOwnedLine(nFd, pstBuffer, &psLine)) {
        return NOSTOS_ERROR;
    }
    if (NOSTOS_OK != parseEndpointLine(psLine, &pstConfig->stListen)) {
        free(psLine);
        return NOSTOS_ERROR;
    }
    free(psLine);
    if (NOSTOS_OK != readOwnedLine(nFd, pstBuffer, &psLine)) {
        return NOSTOS_ERROR;
    }
    if (NOSTOS_OK != parseSingleIntLine(psLine, 0, INT_MAX, &pstConfig->nCapacity)) {
        free(psLine);
        return NOSTOS_ERROR;
    }
    free(psLine);
    if (NOSTOS_OK != readOwnedLine(nFd, pstBuffer, &psLine)) {
        return NOSTOS_ERROR;
    }
    if (0 != strcmp(ROUTES_SECTION_MARKER, psLine)) {
        free(psLine);
        return NOSTOS_ERROR;
    }
    free(psLine);
    return NOSTOS_OK;
}

/***********************************************
 * @Name: routeListAppend
 * @Def: See config.h.
 ***********************************************/
int routeListAppend(tRouteList *pstList, char *psDestination, char *psIp, int nPort) {
    tRoute *pTemp = NULL;
    int nNewCapacity = 0;

    if (pstList->nCount == pstList->nCapacity) {
        if (0 == pstList->nCapacity) {
            nNewCapacity = 4;
        } else {
            nNewCapacity = pstList->nCapacity * 2;
        }
        pTemp = realloc(pstList->pstRoutes, (size_t) nNewCapacity * sizeof(tRoute));
        if (NULL == pTemp) {
            return NOSTOS_ERROR;
        }
        pstList->pstRoutes = pTemp;
        pstList->nCapacity = nNewCapacity;
    }
    pstList->pstRoutes[pstList->nCount].psDestination = psDestination;
    pstList->pstRoutes[pstList->nCount].psIp = psIp;
    pstList->pstRoutes[pstList->nCount].nPort = nPort;
    pstList->nCount++;
    return NOSTOS_OK;
}

/***********************************************
 * @Name: routeListHasDestination
 * @Def: Case-insensitively checks whether a destination name is
 *       already present, defending against a malformed duplicate
 *       route entry silently losing an endpoint (assumption A18).
 * @Arg: In: pstList, psName.
 * @Ret: 1 if present, 0 otherwise.
 ***********************************************/
static int routeListHasDestination(const tRouteList *pstList, const char *psName) {
    int nIndex = 0;

    for (nIndex = 0; nIndex < pstList->nCount; nIndex++) {
        if (0 == strcasecmp(pstList->pstRoutes[nIndex].psDestination, psName)) {
            return 1;
        }
    }
    return 0;
}

/***********************************************
 * @Name: parseRouteLine
 * @Def: Parses a three-token "<NAME> <IP> <PORT>" raw route line.
 * @Arg: In/Out: psLine = mutable line to tokenize.
 *       Out: ppsDestination, ppsIp, pnPort = filled on success.
 * @Ret: NOSTOS_OK / NOSTOS_ERROR.
 ***********************************************/
static int parseRouteLine(char *psLine, char **ppsDestination, char **ppsIp, int *pnPort) {
    char **appsTokens = NULL;
    int nTokenCount = 0;
    long lPort = 0;

    if (NOSTOS_OK != tokenizeLine(psLine, &appsTokens, &nTokenCount)) {
        return NOSTOS_ERROR;
    }
    if (3 != nTokenCount || NOSTOS_OK != parseDigitsToLong(appsTokens[2], &lPort)) {
        free(appsTokens);
        return NOSTOS_ERROR;
    }
    if (lPort < MIN_PORT_NUMBER || lPort > MAX_PORT_NUMBER) {
        free(appsTokens);
        return NOSTOS_ERROR;
    }
    *ppsDestination = duplicateString(appsTokens[0]);
    *ppsIp = duplicateString(appsTokens[1]);
    *pnPort = (int) lPort;
    free(appsTokens);
    if (NULL == *ppsDestination || NULL == *ppsIp) {
        return NOSTOS_ERROR;
    }
    return NOSTOS_OK;
}

/***********************************************
 * @Name: loadIslandRawRoutes
 * @Def: Reads every route line until EOF, rejecting duplicate
 *       destinations defensively (assumption A18).
 * @Arg: In: nFd, In/Out: pstBuffer, Out: pstRawRoutes.
 * @Ret: NOSTOS_OK / NOSTOS_ERROR.
 ***********************************************/
static int loadIslandRawRoutes(int nFd, tLineBuffer *pstBuffer, tRouteList *pstRawRoutes) {
    char *psLine = NULL;
    char *psDestination = NULL;
    char *psIp = NULL;
    int nPort = 0;
    int nReadStatus = 0;

    for (;;) {
        nReadStatus = readNextLine(nFd, pstBuffer, &psLine);
        if (LINE_EOF == nReadStatus) {
            return NOSTOS_OK;
        }
        if (LINE_FOUND != nReadStatus) {
            return NOSTOS_ERROR;
        }
        if (NOSTOS_OK != parseRouteLine(psLine, &psDestination, &psIp, &nPort)) {
            free(psLine);
            free(psDestination);
            free(psIp);
            return NOSTOS_ERROR;
        }
        free(psLine);
        if (routeListHasDestination(pstRawRoutes, psDestination)) {
            free(psDestination);
            free(psIp);
            return NOSTOS_ERROR;
        }
        if (NOSTOS_OK != routeListAppend(pstRawRoutes, psDestination, psIp, nPort)) {
            free(psDestination);
            free(psIp);
            return NOSTOS_ERROR;
        }
    }
}

/***********************************************
 * @Name: failIsland
 * @Def: Single cleanup path for a failed island load.
 * @Arg: In: nFd, In/Out: pstBuffer, pstConfig, pstRawRoutes.
 * @Ret: NOSTOS_ERROR, always.
 ***********************************************/
static int failIsland(int nFd, tLineBuffer *pstBuffer, tIslandConfig *pstConfig, tRouteList *pstRawRoutes) {
    close(nFd);
    lineBufferDestroy(pstBuffer);
    destroyIslandConfig(pstConfig);
    destroyRouteList(pstRawRoutes);
    return NOSTOS_ERROR;
}

/***********************************************
 * @Name: loadIslandConfig
 * @Def: See config.h.
 ***********************************************/
int loadIslandConfig(const char *psPath, tIslandConfig *pstConfig, tRouteList *pstRawRoutes) {
    int nFd = -1;
    tLineBuffer stBuffer;

    initIslandConfig(pstConfig);
    initRouteList(pstRawRoutes);
    lineBufferInit(&stBuffer);
    nFd = safeOpenReadOnly(psPath);
    if (-1 == nFd) {
        return NOSTOS_ERROR;
    }
    if (NOSTOS_OK != loadIslandHeader(nFd, &stBuffer, pstConfig)) {
        return failIsland(nFd, &stBuffer, pstConfig, pstRawRoutes);
    }
    if (NOSTOS_OK != loadIslandRawRoutes(nFd, &stBuffer, pstRawRoutes)) {
        return failIsland(nFd, &stBuffer, pstConfig, pstRawRoutes);
    }
    close(nFd);
    lineBufferDestroy(&stBuffer);
    return NOSTOS_OK;
}

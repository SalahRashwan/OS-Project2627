/*
 * @File: config.c
 * @Purpose: Loaders for the odysseus.dat, ithaca.dat, and island.dat
 *           configuration formats, plus the destructors for the
 *           aggregates they build. Island route validation lives in
 *           routes.c; this file only parses the raw candidate routes.
 * @Author: Salah Ahmed Salaheldin Adly Rashwan
 * @Date: 2026-09-26
 */

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
 * @Arg: In: nFd = open configuration descriptor.
 *       In/Out: pstBuffer = line accumulator reused across calls.
 *       Out: ppsLine = newly owned line on success.
 * @Ret: NOSTOS_OK if a line was produced, NOSTOS_ERROR otherwise.
 ***********************************************/
static int readOwnedLine(int nFd, tLineBuffer *pstBuffer, char **ppsLine) {
    int nResult = readNextLine(nFd, pstBuffer, ppsLine);

    if (LINE_FOUND == nResult) {
        return NOSTOS_OK;
    }
    return NOSTOS_ERROR;
}

/***********************************************
 * @Name: isValidPort
 * @Def: Checks a parsed port value against the accepted TCP range.
 * @Arg: In: lPort = parsed port value.
 * @Ret: 1 if lPort is within [MIN_PORT_NUMBER, MAX_PORT_NUMBER], 0
 *       otherwise.
 ***********************************************/
static int isValidPort(long lPort) {
    if (MIN_PORT_NUMBER > lPort || MAX_PORT_NUMBER < lPort) {
        return 0;
    }
    return 1;
}

/***********************************************
 * @Name: parseEndpointLine
 * @Def: Parses a two-token "<IP> <PORT>" line into an owned endpoint.
 * @Arg: In/Out: psLine = mutable line to tokenize.
 *       Out: pstEndpoint = IP copy and port, filled on success.
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR on a syntax or allocation
 *       failure.
 ***********************************************/
static int parseEndpointLine(char *psLine, tEndpoint *pstEndpoint) {
    char **appsTokens = NULL;
    int nTokenCount = 0;
    long lPort = 0;

    if (NOSTOS_OK != tokenizeLine(psLine, &appsTokens, &nTokenCount)) {
        return NOSTOS_ERROR;
    }
    if (2 != nTokenCount || NOSTOS_OK != parseDigitsToLong(appsTokens[1], &lPort) || 0 == isValidPort(lPort)) {
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
 *       In: lMin = inclusive lower bound.
 *       In: lMax = inclusive upper bound.
 *       Out: pnValue = parsed value, set only on success.
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR on a syntax, range, or
 *       allocation failure.
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
    if (lMin > lValue || lMax < lValue) {
        return NOSTOS_ERROR;
    }
    *pnValue = (int) lValue;
    return NOSTOS_OK;
}

/***********************************************
 * @Name: readOwnedCopyLine
 * @Def: Reads the next mandatory line and stores it as a newly owned
 *       string field (name, storage folder, mission folder).
 * @Arg: In: nFd = open configuration descriptor.
 *       In/Out: pstBuffer = line accumulator.
 *       Out: ppsField = set to the owned line on success; left NULL on
 *            failure.
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR on EOF, read, or allocation
 *       failure.
 ***********************************************/
static int readOwnedCopyLine(int nFd, tLineBuffer *pstBuffer, char **ppsField) {
    char *psLine = NULL;

    if (NOSTOS_OK != readOwnedLine(nFd, pstBuffer, &psLine)) {
        return NOSTOS_ERROR;
    }
    /* The line is already a fresh owned allocation; move it directly. */
    *ppsField = psLine;
    return NOSTOS_OK;
}

/***********************************************
 * @Name: readEndpointField
 * @Def: Reads the next mandatory line and parses it as "<IP> <PORT>".
 * @Arg: In: nFd = open configuration descriptor.
 *       In/Out: pstBuffer = line accumulator.
 *       Out: pstEndpoint = filled on success.
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR otherwise.
 ***********************************************/
static int readEndpointField(int nFd, tLineBuffer *pstBuffer, tEndpoint *pstEndpoint) {
    char *psLine = NULL;
    int nStatus = NOSTOS_ERROR;

    if (NOSTOS_OK != readOwnedLine(nFd, pstBuffer, &psLine)) {
        return NOSTOS_ERROR;
    }
    nStatus = parseEndpointLine(psLine, pstEndpoint);
    free(psLine);
    return nStatus;
}

/***********************************************
 * @Name: readIntField
 * @Def: Reads the next mandatory line and parses it as one bounded
 *       integer.
 * @Arg: In: nFd = open configuration descriptor.
 *       In/Out: pstBuffer = line accumulator.
 *       In: lMin = inclusive lower bound.
 *       In: lMax = inclusive upper bound.
 *       Out: pnValue = parsed value on success.
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR otherwise.
 ***********************************************/
static int readIntField(int nFd, tLineBuffer *pstBuffer, long lMin, long lMax, int *pnValue) {
    char *psLine = NULL;
    int nStatus = NOSTOS_ERROR;

    if (NOSTOS_OK != readOwnedLine(nFd, pstBuffer, &psLine)) {
        return NOSTOS_ERROR;
    }
    nStatus = parseSingleIntLine(psLine, lMin, lMax, pnValue);
    free(psLine);
    return nStatus;
}

/* ---- Odysseus configuration ---------------------------------------- */

/***********************************************
 * @Name: initOdysseusConfig
 * @Def: Resets an Odysseus configuration to a safe, destroyable state.
 * @Arg: Out: pstConfig = configuration to reset.
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
 * @Def: Frees every owned allocation inside an Odysseus configuration
 *       (strings, endpoint IPs, and the fully owned food entries), then
 *       resets it so a second call is harmless.
 * @Arg: In/Out: pstConfig = configuration to release; may be zeroed or
 *       partially filled.
 * @Ret: None.
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
 *       Out: ppsName = owned island name (may be set even on a later
 *            allocation failure; the caller's destructor frees it).
 *       Out: pstEndpoint = owned IP and port.
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR otherwise.
 ***********************************************/
static int parseInitialIslandLine(char *psLine, char **ppsName, tEndpoint *pstEndpoint) {
    char **appsTokens = NULL;
    int nTokenCount = 0;
    long lPort = 0;

    if (NOSTOS_OK != tokenizeLine(psLine, &appsTokens, &nTokenCount)) {
        return NOSTOS_ERROR;
    }
    if (3 != nTokenCount || NOSTOS_OK != parseDigitsToLong(appsTokens[2], &lPort) || 0 == isValidPort(lPort)) {
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
 *       Out: pstFood = product copy and amount, filled on success;
 *            psProduct stays NULL on failure.
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR otherwise.
 ***********************************************/
static int parseFoodLine(char *psLine, tFoodEntry *pstFood) {
    char **appsTokens = NULL;
    int nTokenCount = 0;
    long lAmount = 0;

    pstFood->psProduct = NULL;
    if (NOSTOS_OK != tokenizeLine(psLine, &appsTokens, &nTokenCount)) {
        return NOSTOS_ERROR;
    }
    if (2 != nTokenCount || NOSTOS_OK != parseDigitsToLong(appsTokens[1], &lAmount) || INT_MAX < lAmount) {
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
 * @Arg: In: nFd = open configuration descriptor.
 *       In/Out: pstBuffer = line accumulator.
 *       Out: pstConfig = receives the three fields.
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR otherwise.
 ***********************************************/
static int loadOdysseusIdentity(int nFd, tLineBuffer *pstBuffer, tOdysseusConfig *pstConfig) {
    if (NOSTOS_OK != readOwnedCopyLine(nFd, pstBuffer, &pstConfig->psName)) {
        return NOSTOS_ERROR;
    }
    if (NOSTOS_OK != readOwnedCopyLine(nFd, pstBuffer, &pstConfig->psStorageFolder)) {
        return NOSTOS_ERROR;
    }
    return readEndpointField(nFd, pstBuffer, &pstConfig->stIthaca);
}

/***********************************************
 * @Name: loadOdysseusResources
 * @Def: Reads the initial island, gold, and the declared food-entry
 *       count, in that order.
 * @Arg: In: nFd = open configuration descriptor.
 *       In/Out: pstBuffer = line accumulator.
 *       Out: pstConfig = receives initial island and gold.
 *       Out: pnFoodCount = declared number of food lines that follow.
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR otherwise.
 ***********************************************/
static int loadOdysseusResources(int nFd, tLineBuffer *pstBuffer, tOdysseusConfig *pstConfig, int *pnFoodCount) {
    char *psLine = NULL;
    int nStatus = NOSTOS_ERROR;

    if (NOSTOS_OK != readOwnedLine(nFd, pstBuffer, &psLine)) {
        return NOSTOS_ERROR;
    }
    nStatus = parseInitialIslandLine(psLine, &pstConfig->psInitialIsland, &pstConfig->stInitialIslandEndpoint);
    free(psLine);
    if (NOSTOS_OK != nStatus) {
        return NOSTOS_ERROR;
    }
    if (NOSTOS_OK != readIntField(nFd, pstBuffer, 0, INT_MAX, &pstConfig->nGold)) {
        return NOSTOS_ERROR;
    }
    return readIntField(nFd, pstBuffer, 0, INT_MAX, pnFoodCount);
}

/***********************************************
 * @Name: loadOdysseusFoods
 * @Def: Allocates and fills exactly nFoodCount food entries. The owned
 *       count only grows after an entry is complete, so the destructor
 *       never frees an unfilled slot.
 * @Arg: In: nFd = open configuration descriptor.
 *       In/Out: pstBuffer = line accumulator.
 *       Out: pstConfig = receives the food array and count.
 *       In: nFoodCount = number of food lines declared by the file.
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR otherwise.
 ***********************************************/
static int loadOdysseusFoods(int nFd, tLineBuffer *pstBuffer, tOdysseusConfig *pstConfig, int nFoodCount) {
    char *psLine = NULL;
    int nIndex = 0;
    int nStatus = NOSTOS_ERROR;

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
        nStatus = parseFoodLine(psLine, &pstConfig->pstFoods[nIndex]);
        free(psLine);
        if (NOSTOS_OK != nStatus) {
            return NOSTOS_ERROR;
        }
        pstConfig->nFoodCount = nIndex + 1;
    }
    return NOSTOS_OK;
}

/***********************************************
 * @Name: failOdysseus
 * @Def: Single cleanup path for a failed Odysseus load.
 * @Arg: In: nFd = descriptor to close.
 *       In/Out: pstBuffer = line accumulator to release.
 *       In/Out: pstConfig = partially loaded configuration to release.
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
 * @Def: Opens, parses, and closes an Odysseus configuration file.
 * @Arg: In: psPath = path to the configuration file.
 *       Out: pstConfig = filled on success; released and reset to a
 *            safe, destroyable state on failure.
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR on open, read, syntax, or
 *       allocation failure.
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
 * @Arg: Out: pstConfig = configuration to reset.
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
 * @Def: Frees every owned allocation inside an Ithaca configuration and
 *       resets it so a second call is harmless.
 * @Arg: In/Out: pstConfig = configuration to release.
 * @Ret: None.
 ***********************************************/
void destroyIthacaConfig(tIthacaConfig *pstConfig) {
    free(pstConfig->psName);
    free(pstConfig->psMissionFolder);
    free(pstConfig->stListen.psIp);
    initIthacaConfig(pstConfig);
}

/***********************************************
 * @Name: loadIthacaConfig
 * @Def: Opens, parses, and closes an Ithaca configuration file (name,
 *       mission folder, listen endpoint).
 * @Arg: In: psPath = path to the configuration file.
 *       Out: pstConfig = filled on success; released and reset on
 *            failure.
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR otherwise.
 ***********************************************/
int loadIthacaConfig(const char *psPath, tIthacaConfig *pstConfig) {
    int nFd = -1;
    tLineBuffer stBuffer;
    int nStatus = NOSTOS_ERROR;

    initIthacaConfig(pstConfig);
    lineBufferInit(&stBuffer);
    nFd = safeOpenReadOnly(psPath);
    if (-1 == nFd) {
        return NOSTOS_ERROR;
    }
    nStatus = readOwnedCopyLine(nFd, &stBuffer, &pstConfig->psName);
    if (NOSTOS_OK == nStatus) {
        nStatus = readOwnedCopyLine(nFd, &stBuffer, &pstConfig->psMissionFolder);
    }
    if (NOSTOS_OK == nStatus) {
        nStatus = readEndpointField(nFd, &stBuffer, &pstConfig->stListen);
    }
    close(nFd);
    lineBufferDestroy(&stBuffer);
    if (NOSTOS_OK != nStatus) {
        destroyIthacaConfig(pstConfig);
        return NOSTOS_ERROR;
    }
    return NOSTOS_OK;
}

/* ---- Island configuration and raw (unvalidated) routes --------------- */

/***********************************************
 * @Name: initIslandConfig
 * @Def: Resets an island configuration to a safe, destroyable state.
 * @Arg: Out: pstConfig = configuration to reset.
 * @Ret: None.
 ***********************************************/
static void initIslandConfig(tIslandConfig *pstConfig) {
    pstConfig->psName = NULL;
    pstConfig->psStorageFolder = NULL;
    pstConfig->stListen.psIp = NULL;
    pstConfig->stListen.nPort = 0;
    pstConfig->nCapacity = 0;
    initRouteList(&pstConfig->stRoutes);
}

/***********************************************
 * @Name: initRouteList
 * @Def: Resets a route list to a safe, destroyable empty state. Used
 *       for both the raw candidate list and the post-filter valid list.
 * @Arg: Out: pstList = list to initialize.
 * @Ret: None.
 ***********************************************/
void initRouteList(tRouteList *pstList) {
    pstList->pstRoutes = NULL;
    pstList->nCount = 0;
    pstList->nCapacity = 0;
}

/***********************************************
 * @Name: destroyRouteList
 * @Def: Frees every owned destination/IP string of the first nCount
 *       routes (the only fully owned entries) and the array itself.
 * @Arg: In/Out: pstList = route list to release; reset afterwards.
 * @Ret: None.
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
 * @Def: Frees every owned allocation inside an island configuration,
 *       including its (post-filtering) valid route list.
 * @Arg: In/Out: pstConfig = configuration to release; reset afterwards.
 * @Ret: None.
 ***********************************************/
void destroyIslandConfig(tIslandConfig *pstConfig) {
    free(pstConfig->psName);
    free(pstConfig->psStorageFolder);
    free(pstConfig->stListen.psIp);
    destroyRouteList(&pstConfig->stRoutes);
    initIslandConfig(pstConfig);
}

/***********************************************
 * @Name: readRoutesMarker
 * @Def: Reads the next mandatory line and checks that it is exactly the
 *       routes-section marker.
 * @Arg: In: nFd = open configuration descriptor.
 *       In/Out: pstBuffer = line accumulator.
 * @Ret: NOSTOS_OK if the marker is present, NOSTOS_ERROR otherwise.
 ***********************************************/
static int readRoutesMarker(int nFd, tLineBuffer *pstBuffer) {
    char *psLine = NULL;
    int nStatus = NOSTOS_ERROR;

    if (NOSTOS_OK != readOwnedLine(nFd, pstBuffer, &psLine)) {
        return NOSTOS_ERROR;
    }
    if (0 == strcmp(ROUTES_SECTION_MARKER, psLine)) {
        nStatus = NOSTOS_OK;
    }
    free(psLine);
    return nStatus;
}

/***********************************************
 * @Name: loadIslandHeader
 * @Def: Reads name, storage folder, listen endpoint, port capacity,
 *       and the exact routes-section marker, in that order.
 * @Arg: In: nFd = open configuration descriptor.
 *       In/Out: pstBuffer = line accumulator.
 *       Out: pstConfig = receives the header fields.
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR otherwise.
 ***********************************************/
static int loadIslandHeader(int nFd, tLineBuffer *pstBuffer, tIslandConfig *pstConfig) {
    if (NOSTOS_OK != readOwnedCopyLine(nFd, pstBuffer, &pstConfig->psName)) {
        return NOSTOS_ERROR;
    }
    if (NOSTOS_OK != readOwnedCopyLine(nFd, pstBuffer, &pstConfig->psStorageFolder)) {
        return NOSTOS_ERROR;
    }
    if (NOSTOS_OK != readEndpointField(nFd, pstBuffer, &pstConfig->stListen)) {
        return NOSTOS_ERROR;
    }
    if (NOSTOS_OK != readIntField(nFd, pstBuffer, 0, INT_MAX, &pstConfig->nCapacity)) {
        return NOSTOS_ERROR;
    }
    return readRoutesMarker(nFd, pstBuffer);
}

/***********************************************
 * @Name: routeListAppend
 * @Def: Appends one already-owned route to a growable route list,
 *       moving ownership of its strings into the list on success. A
 *       failed realloc leaves the existing array and its owner intact.
 * @Arg: In/Out: pstList = list to grow.
 *       In: psDestination = owned destination name to move in.
 *       In: psIp = owned IP text to move in.
 *       In: nPort = route port.
 * @Ret: NOSTOS_OK on success (ownership moved); NOSTOS_ERROR on
 *       allocation failure (caller still owns and must free the
 *       strings).
 ***********************************************/
int routeListAppend(tRouteList *pstList, char *psDestination, char *psIp, int nPort) {
    tRoute *pstTemp = NULL;
    int nNewCapacity = 0;

    if (pstList->nCount == pstList->nCapacity) {
        if (0 == pstList->nCapacity) {
            nNewCapacity = 4;
        } else {
            nNewCapacity = pstList->nCapacity * 2;
        }
        pstTemp = realloc(pstList->pstRoutes, (size_t) nNewCapacity * sizeof(tRoute));
        if (NULL == pstTemp) {
            return NOSTOS_ERROR;
        }
        pstList->pstRoutes = pstTemp;
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
 * @Arg: In: pstList = list to search.
 *       In: psName = destination name to look for.
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
 *       Out: ppsDestination = owned destination copy (may be set even
 *            when a later allocation fails).
 *       Out: ppsIp = owned IP copy (same rule).
 *       Out: pnPort = parsed port.
 *       The caller must initialize both string outputs to NULL and free
 *       whatever is non-NULL after a failure.
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR otherwise.
 ***********************************************/
static int parseRouteLine(char *psLine, char **ppsDestination, char **ppsIp, int *pnPort) {
    char **appsTokens = NULL;
    int nTokenCount = 0;
    long lPort = 0;

    if (NOSTOS_OK != tokenizeLine(psLine, &appsTokens, &nTokenCount)) {
        return NOSTOS_ERROR;
    }
    if (3 != nTokenCount || NOSTOS_OK != parseDigitsToLong(appsTokens[2], &lPort) || 0 == isValidPort(lPort)) {
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
 * @Name: appendRawRouteLine
 * @Def: Parses one route line into fresh, iteration-local strings and
 *       moves them into the raw route list. Every error branch frees
 *       only the strings created by this call, so strings already moved
 *       into the list by earlier calls are never freed here.
 * @Arg: In/Out: psLine = mutable route line (borrowed, not freed).
 *       In/Out: pstRawRoutes = list that receives the route.
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR on syntax, duplicate, or
 *       allocation failure.
 ***********************************************/
static int appendRawRouteLine(char *psLine, tRouteList *pstRawRoutes) {
    char *psDestination = NULL;
    char *psIp = NULL;
    int nPort = 0;

    if (NOSTOS_OK != parseRouteLine(psLine, &psDestination, &psIp, &nPort)) {
        free(psDestination);
        free(psIp);
        return NOSTOS_ERROR;
    }
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
    return NOSTOS_OK;
}

/***********************************************
 * @Name: loadIslandRawRoutes
 * @Def: Reads every route line until EOF and appends each one to the
 *       raw candidate list.
 * @Arg: In: nFd = open configuration descriptor.
 *       In/Out: pstBuffer = line accumulator.
 *       Out: pstRawRoutes = receives every candidate route in file order.
 * @Ret: NOSTOS_OK on clean EOF, NOSTOS_ERROR otherwise.
 ***********************************************/
static int loadIslandRawRoutes(int nFd, tLineBuffer *pstBuffer, tRouteList *pstRawRoutes) {
    char *psLine = NULL;
    int nReadStatus = 0;
    int nStatus = NOSTOS_OK;

    for (;;) {
        nReadStatus = readNextLine(nFd, pstBuffer, &psLine);
        if (LINE_EOF == nReadStatus) {
            return NOSTOS_OK;
        }
        if (LINE_FOUND != nReadStatus) {
            return NOSTOS_ERROR;
        }
        nStatus = appendRawRouteLine(psLine, pstRawRoutes);
        free(psLine);
        if (NOSTOS_OK != nStatus) {
            return NOSTOS_ERROR;
        }
    }
}

/***********************************************
 * @Name: loadIslandConfig
 * @Def: Opens, parses, and closes an island configuration file,
 *       including its raw (not yet Sphragis-validated) routes section.
 * @Arg: In: psPath = path to the configuration file.
 *       Out: pstConfig = filled with name/folder/endpoint/capacity; its
 *            stRoutes list is left empty for routes.c to fill.
 *       Out: pstRawRoutes = every candidate route in file order, owned
 *            by the caller.
 *       Both outputs are released and reset on failure.
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR otherwise.
 ***********************************************/
int loadIslandConfig(const char *psPath, tIslandConfig *pstConfig, tRouteList *pstRawRoutes) {
    int nFd = -1;
    tLineBuffer stBuffer;
    int nStatus = NOSTOS_ERROR;

    initIslandConfig(pstConfig);
    initRouteList(pstRawRoutes);
    lineBufferInit(&stBuffer);
    nFd = safeOpenReadOnly(psPath);
    if (-1 == nFd) {
        return NOSTOS_ERROR;
    }
    nStatus = loadIslandHeader(nFd, &stBuffer, pstConfig);
    if (NOSTOS_OK == nStatus) {
        nStatus = loadIslandRawRoutes(nFd, &stBuffer, pstRawRoutes);
    }
    close(nFd);
    lineBufferDestroy(&stBuffer);
    if (NOSTOS_OK != nStatus) {
        destroyIslandConfig(pstConfig);
        destroyRouteList(pstRawRoutes);
        return NOSTOS_ERROR;
    }
    return NOSTOS_OK;
}

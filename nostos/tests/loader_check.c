/*
 * @File: loader_check.c
 * @Purpose: Test-only diagnostic tool (built by `make test`, never linked
 *           into odysseus/ithaca/island). Links the real loaders and the
 *           real Sphragis adapter and prints every loaded field, one per
 *           line, so tests/run_functional_tests.sh can compare them with
 *           values decoded independently from the input files. Follows
 *           the same descriptor-only I/O and style rules as the runtime.
 *           Usage: loader_check odysseus <config>
 *                  loader_check ithaca <config> <voyages>
 *                  loader_check island <config> <stock>
 *           Exit status: 0 = every load succeeded, 1 = usage error,
 *           2 = a loader failed.
 * @Author: Salah Ahmed Salaheldin Adly Rashwan
 * @Date: 2026-09-26
 */

/* Own */
#include "config.h"
#include "voyages.h"
#include "stock.h"
#include "routes.h"
#include "io.h"
#include "status.h"

/***********************************************
 * @Name: dumpOdysseus
 * @Def: Loads an Odysseus configuration and prints every field.
 * @Arg: In: psPath = path to odysseus.dat.
 * @Ret: NOSTOS_OK if the load and every write succeeded, NOSTOS_ERROR
 *       otherwise.
 ***********************************************/
static int dumpOdysseus(const char *psPath) {
    tOdysseusConfig stConfig;
    int nIndex = 0;
    int nStatus = NOSTOS_OK;

    if (NOSTOS_OK != loadOdysseusConfig(psPath, &stConfig)) {
        return NOSTOS_ERROR;
    }
    nStatus |= writeFormatted(STDOUT_FILENO, "odysseus.name=%s\n", stConfig.psName);
    nStatus |= writeFormatted(STDOUT_FILENO, "odysseus.storageFolder=%s\n", stConfig.psStorageFolder);
    nStatus |= writeFormatted(STDOUT_FILENO, "odysseus.ithaca=%s:%d\n", stConfig.stIthaca.psIp,
                              stConfig.stIthaca.nPort);
    nStatus |= writeFormatted(STDOUT_FILENO, "odysseus.initialIsland=%s %s:%d\n", stConfig.psInitialIsland,
                              stConfig.stInitialIslandEndpoint.psIp, stConfig.stInitialIslandEndpoint.nPort);
    nStatus |= writeFormatted(STDOUT_FILENO, "odysseus.gold=%d\n", stConfig.nGold);
    nStatus |= writeFormatted(STDOUT_FILENO, "odysseus.foodCount=%d\n", stConfig.nFoodCount);
    for (nIndex = 0; nIndex < stConfig.nFoodCount; nIndex++) {
        nStatus |= writeFormatted(STDOUT_FILENO, "odysseus.food[%d]=%s %d\n", nIndex,
                                  stConfig.pstFoods[nIndex].psProduct, stConfig.pstFoods[nIndex].nAmountKg);
    }
    destroyOdysseusConfig(&stConfig);
    if (NOSTOS_OK != nStatus) {
        return NOSTOS_ERROR;
    }
    return NOSTOS_OK;
}

/***********************************************
 * @Name: dumpVoyages
 * @Def: Prints every voyage record with its internal identifier.
 * @Arg: In: pstVoyages = loaded voyage list.
 * @Ret: NOSTOS_OK if every write succeeded, NOSTOS_ERROR otherwise.
 ***********************************************/
static int dumpVoyages(const tVoyageList *pstVoyages) {
    int nIndex = 0;
    int nStatus = NOSTOS_OK;
    const tVoyage *pstVoyage = NULL;

    nStatus |= writeFormatted(STDOUT_FILENO, "ithaca.voyageCount=%d\n", pstVoyages->nCount);
    for (nIndex = 0; nIndex < pstVoyages->nCount; nIndex++) {
        pstVoyage = &pstVoyages->pstVoyages[nIndex];
        nStatus |= writeFormatted(STDOUT_FILENO, "ithaca.voyage[%d]=id=%d object=%s file=%s dest=%s reward=%d\n",
                                  nIndex, pstVoyage->nId, pstVoyage->psObject, pstVoyage->psFilePath,
                                  pstVoyage->psDestination, pstVoyage->nReward);
    }
    if (NOSTOS_OK != nStatus) {
        return NOSTOS_ERROR;
    }
    return NOSTOS_OK;
}

/***********************************************
 * @Name: dumpIthaca
 * @Def: Loads an Ithaca configuration and voyage list and prints every
 *       field of both.
 * @Arg: In: psConfigPath = path to ithaca.dat.
 *       In: psVoyagesPath = path to voyages.dat.
 * @Ret: NOSTOS_OK if both loads and every write succeeded, NOSTOS_ERROR
 *       otherwise.
 ***********************************************/
static int dumpIthaca(const char *psConfigPath, const char *psVoyagesPath) {
    tIthacaConfig stConfig;
    tVoyageList stVoyages;
    int nStatus = NOSTOS_OK;

    if (NOSTOS_OK != loadIthacaConfig(psConfigPath, &stConfig)) {
        return NOSTOS_ERROR;
    }
    nStatus |= writeFormatted(STDOUT_FILENO, "ithaca.name=%s\n", stConfig.psName);
    nStatus |= writeFormatted(STDOUT_FILENO, "ithaca.missionFolder=%s\n", stConfig.psMissionFolder);
    nStatus |= writeFormatted(STDOUT_FILENO, "ithaca.listen=%s:%d\n", stConfig.stListen.psIp,
                              stConfig.stListen.nPort);
    destroyIthacaConfig(&stConfig);
    if (NOSTOS_OK != loadVoyages(psVoyagesPath, &stVoyages)) {
        return NOSTOS_ERROR;
    }
    nStatus |= dumpVoyages(&stVoyages);
    destroyVoyageList(&stVoyages);
    if (NOSTOS_OK != nStatus) {
        return NOSTOS_ERROR;
    }
    return NOSTOS_OK;
}

/***********************************************
 * @Name: dumpRouteList
 * @Def: Prints every route of a list under a given label.
 * @Arg: In: psLabel = line prefix ("rawRoute" or "validRoute").
 *       In: pstList = routes to print.
 * @Ret: NOSTOS_OK if every write succeeded, NOSTOS_ERROR otherwise.
 ***********************************************/
static int dumpRouteList(const char *psLabel, const tRouteList *pstList) {
    int nIndex = 0;
    int nStatus = NOSTOS_OK;

    nStatus |= writeFormatted(STDOUT_FILENO, "island.%sCount=%d\n", psLabel, pstList->nCount);
    for (nIndex = 0; nIndex < pstList->nCount; nIndex++) {
        nStatus |= writeFormatted(STDOUT_FILENO, "island.%s[%d]=%s %s:%d\n", psLabel, nIndex,
                                  pstList->pstRoutes[nIndex].psDestination, pstList->pstRoutes[nIndex].psIp,
                                  pstList->pstRoutes[nIndex].nPort);
    }
    if (NOSTOS_OK != nStatus) {
        return NOSTOS_ERROR;
    }
    return NOSTOS_OK;
}

/***********************************************
 * @Name: dumpStock
 * @Def: Loads a binary stock file and prints every record.
 * @Arg: In: psStockPath = path to the stock.db file.
 * @Ret: NOSTOS_OK if the load and every write succeeded, NOSTOS_ERROR
 *       otherwise.
 ***********************************************/
static int dumpStock(const char *psStockPath) {
    tStockList stStock;
    int nIndex = 0;
    int nStatus = NOSTOS_OK;
    char *psName = NULL;

    if (NOSTOS_OK != loadStockList(psStockPath, &stStock)) {
        return NOSTOS_ERROR;
    }
    nStatus |= writeFormatted(STDOUT_FILENO, "island.productCount=%d\n", stStock.nCount);
    for (nIndex = 0; nIndex < stStock.nCount && NOSTOS_OK == nStatus; nIndex++) {
        psName = stockRecordNameToString(&stStock.pstRecords[nIndex]);
        if (NULL == psName) {
            nStatus = NOSTOS_ERROR;
        } else {
            nStatus |= writeFormatted(STDOUT_FILENO, "island.product[%d]=%s amount=%d price=%d\n", nIndex, psName,
                                      stStock.pstRecords[nIndex].nAmount, stStock.pstRecords[nIndex].nPrice);
            free(psName);
        }
    }
    destroyStockList(&stStock);
    if (NOSTOS_OK != nStatus) {
        return NOSTOS_ERROR;
    }
    return NOSTOS_OK;
}

/***********************************************
 * @Name: dumpIsland
 * @Def: Loads an island configuration, prints its fields and raw
 *       routes, filters them through the real Sphragis adapter, prints
 *       the result and surviving routes, then dumps its stock.
 * @Arg: In: psConfigPath = path to the island configuration.
 *       In: psStockPath = path to the stock.db file.
 * @Ret: NOSTOS_OK if every step succeeded, NOSTOS_ERROR otherwise.
 ***********************************************/
static int dumpIsland(const char *psConfigPath, const char *psStockPath) {
    tIslandConfig stConfig;
    tRouteList stRawRoutes;
    int nFilterResult = 0;
    int nStatus = NOSTOS_OK;

    if (NOSTOS_OK != loadIslandConfig(psConfigPath, &stConfig, &stRawRoutes)) {
        return NOSTOS_ERROR;
    }
    nStatus |= writeFormatted(STDOUT_FILENO, "island.name=%s\n", stConfig.psName);
    nStatus |= writeFormatted(STDOUT_FILENO, "island.storageFolder=%s\n", stConfig.psStorageFolder);
    nStatus |= writeFormatted(STDOUT_FILENO, "island.listen=%s:%d\n", stConfig.stListen.psIp,
                              stConfig.stListen.nPort);
    nStatus |= writeFormatted(STDOUT_FILENO, "island.capacity=%d\n", stConfig.nCapacity);
    nStatus |= dumpRouteList("rawRoute", &stRawRoutes);
    nFilterResult = filterIslandRoutes(stConfig.psName, &stRawRoutes, &stConfig.stRoutes);
    destroyRouteList(&stRawRoutes);
    nStatus |= writeFormatted(STDOUT_FILENO, "island.sphragisResult=%d\n", nFilterResult);
    nStatus |= dumpRouteList("validRoute", &stConfig.stRoutes);
    destroyIslandConfig(&stConfig);
    if (0 > nFilterResult || NOSTOS_OK != nStatus) {
        return NOSTOS_ERROR;
    }
    return dumpStock(psStockPath);
}

/***********************************************
 * @Name: main
 * @Def: Dispatches to the requested dump.
 * @Arg: In: argc = argument count.
 *       In: argv = argument vector: mode followed by its paths.
 * @Ret: 0 on success, 1 on a usage error, 2 if a load or write failed.
 ***********************************************/
int main(int argc, char *argv[]) {
    int nStatus = NOSTOS_ERROR;

    if (3 == argc && 0 == strcmp("odysseus", argv[1])) {
        nStatus = dumpOdysseus(argv[2]);
    } else if (4 == argc && 0 == strcmp("ithaca", argv[1])) {
        nStatus = dumpIthaca(argv[2], argv[3]);
    } else if (4 == argc && 0 == strcmp("island", argv[1])) {
        nStatus = dumpIsland(argv[2], argv[3]);
    } else {
        (void) writeString(STDERR_FILENO, "Usage: loader_check odysseus <cfg> | ithaca <cfg> <voyages> | "
                                          "island <cfg> <stock>\n");
        return 1;
    }
    if (NOSTOS_OK != nStatus) {
        (void) writeString(STDERR_FILENO, "loader_check: load failed\n");
        return 2;
    }
    return 0;
}

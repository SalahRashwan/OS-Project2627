/*
 * @File: loader_check.c
 * @Purpose: TEST-ONLY diagnostic tool, not part of the graded/delivered
 *           odysseus/ithaca/island executables and not built by `make
 *           all`. Links the real loaders and dumps every loaded field,
 *           to prove field-level storage rather than just startup
 *           messages (guide section 14.4.1). Uses plain printf/fprintf
 *           because it is a verification harness, not authored runtime
 *           code subject to the project's I/O restriction; see
 *           docs/test-results.md for how it is invoked and why it is
 *           exempt from the forbidden-API audit.
 * @Author: Salah Ahmed Salaheldin Adly Rashwan
 * @Date: 2026-09-21
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "voyages.h"
#include "stock.h"
#include "routes.h"
#include "status.h"

static void dumpOdysseus(const char *psPath) {
    tOdysseusConfig stConfig;
    int nIndex = 0;

    if (NOSTOS_OK != loadOdysseusConfig(psPath, &stConfig)) {
        printf("odysseus: LOAD FAILED\n");
        return;
    }
    printf("odysseus.name=%s\n", stConfig.psName);
    printf("odysseus.storageFolder=%s\n", stConfig.psStorageFolder);
    printf("odysseus.ithaca=%s:%d\n", stConfig.stIthaca.psIp, stConfig.stIthaca.nPort);
    printf("odysseus.initialIsland=%s %s:%d\n", stConfig.psInitialIsland,
           stConfig.stInitialIslandEndpoint.psIp, stConfig.stInitialIslandEndpoint.nPort);
    printf("odysseus.gold=%d\n", stConfig.nGold);
    printf("odysseus.foodCount=%d\n", stConfig.nFoodCount);
    for (nIndex = 0; nIndex < stConfig.nFoodCount; nIndex++) {
        printf("odysseus.food[%d]=%s %d\n", nIndex, stConfig.pstFoods[nIndex].psProduct,
               stConfig.pstFoods[nIndex].nAmountKg);
    }
    destroyOdysseusConfig(&stConfig);
}

static void dumpIthaca(const char *psConfigPath, const char *psVoyagesPath) {
    tIthacaConfig stConfig;
    tVoyageList stVoyages;
    int nIndex = 0;

    if (NOSTOS_OK != loadIthacaConfig(psConfigPath, &stConfig)) {
        printf("ithaca: CONFIG LOAD FAILED\n");
        return;
    }
    printf("ithaca.name=%s\n", stConfig.psName);
    printf("ithaca.missionFolder=%s\n", stConfig.psMissionFolder);
    printf("ithaca.listen=%s:%d\n", stConfig.stListen.psIp, stConfig.stListen.nPort);
    if (NOSTOS_OK != loadVoyages(psVoyagesPath, &stVoyages)) {
        printf("ithaca: VOYAGES LOAD FAILED\n");
        destroyIthacaConfig(&stConfig);
        return;
    }
    printf("ithaca.voyageCount=%d\n", stVoyages.nCount);
    for (nIndex = 0; nIndex < stVoyages.nCount; nIndex++) {
        printf("ithaca.voyage[%d]=id=%d object=%s file=%s dest=%s reward=%d\n", nIndex,
               stVoyages.pstVoyages[nIndex].nId, stVoyages.pstVoyages[nIndex].psObject,
               stVoyages.pstVoyages[nIndex].psFilePath, stVoyages.pstVoyages[nIndex].psDestination,
               stVoyages.pstVoyages[nIndex].nReward);
    }
    destroyVoyageList(&stVoyages);
    destroyIthacaConfig(&stConfig);
}

static void dumpIsland(const char *psConfigPath, const char *psStockPath) {
    tIslandConfig stConfig;
    tRouteList stRawRoutes;
    tStockList stStock;
    int nIndex = 0;
    int nFilterResult = 0;
    char *psName = NULL;

    if (NOSTOS_OK != loadIslandConfig(psConfigPath, &stConfig, &stRawRoutes)) {
        printf("island: CONFIG LOAD FAILED\n");
        return;
    }
    printf("island.name=%s\n", stConfig.psName);
    printf("island.storageFolder=%s\n", stConfig.psStorageFolder);
    printf("island.listen=%s:%d\n", stConfig.stListen.psIp, stConfig.stListen.nPort);
    printf("island.capacity=%d\n", stConfig.nCapacity);
    printf("island.rawRouteCount=%d\n", stRawRoutes.nCount);
    for (nIndex = 0; nIndex < stRawRoutes.nCount; nIndex++) {
        printf("island.rawRoute[%d]=%s %s:%d\n", nIndex, stRawRoutes.pstRoutes[nIndex].psDestination,
               stRawRoutes.pstRoutes[nIndex].psIp, stRawRoutes.pstRoutes[nIndex].nPort);
    }
    nFilterResult = filterIslandRoutes(stConfig.psName, &stRawRoutes, &stConfig.stRoutes);
    destroyRouteList(&stRawRoutes);
    printf("island.sphragisResult=%d\n", nFilterResult);
    for (nIndex = 0; nIndex < stConfig.stRoutes.nCount; nIndex++) {
        printf("island.validRoute[%d]=%s %s:%d\n", nIndex, stConfig.stRoutes.pstRoutes[nIndex].psDestination,
               stConfig.stRoutes.pstRoutes[nIndex].psIp, stConfig.stRoutes.pstRoutes[nIndex].nPort);
    }
    if (NOSTOS_OK != loadStockList(psStockPath, &stStock)) {
        printf("island: STOCK LOAD FAILED\n");
        destroyIslandConfig(&stConfig);
        return;
    }
    printf("island.productCount=%d\n", stStock.nCount);
    for (nIndex = 0; nIndex < stStock.nCount; nIndex++) {
        psName = stockRecordNameToString(&stStock.pstRecords[nIndex]);
        printf("island.product[%d]=%s amount=%d price=%d\n", nIndex, psName, stStock.pstRecords[nIndex].nAmount,
               stStock.pstRecords[nIndex].nPrice);
        free(psName);
    }
    destroyStockList(&stStock);
    destroyIslandConfig(&stConfig);
}

int main(int argc, char *argv[]) {
    if (3 == argc && 0 == strcmp("odysseus", argv[1])) {
        dumpOdysseus(argv[2]);
        return 0;
    }
    if (4 == argc && 0 == strcmp("ithaca", argv[1])) {
        dumpIthaca(argv[2], argv[3]);
        return 0;
    }
    if (4 == argc && 0 == strcmp("island", argv[1])) {
        dumpIsland(argv[2], argv[3]);
        return 0;
    }
    fprintf(stderr, "Usage: loader_check odysseus <cfg> | ithaca <cfg> <voyages> | island <cfg> <stock>\n");
    return 1;
}

/*
 * @File: Island.c
 * @Purpose: Entry point for the Island process. Usage:
 *           ./island <config.dat> <stock.db>
 *           One executable represents any island: it loads that
 *           island's identity, raw candidate routes, and binary
 *           stock, mandatorily validates the routes through the real
 *           SPHRAGIS_filter_island_configuration(), announces
 *           readiness, then blocks (no busy waiting) until CTRL+C,
 *           releasing every owned resource before exiting. Island has
 *           no interactive terminal (P p.17).
 * @Author: Salah Ahmed Salaheldin Adly Rashwan
 * @Date: 2026-09-21
 */

#include "Island.h"

/***********************************************
 * @Name: loadIslandRoutesFiltered
 * @Def: Loads the raw configuration/routes, then mandatorily filters
 *       the routes through the real Sphragis adapter before they are
 *       ever considered valid. Self-cleaning: on any failure the
 *       island configuration is fully released before returning.
 * @Arg: In: psConfigPath. Out: pstConfig (its stRoutes becomes the
 *       valid, post-filter list on success).
 * @Ret: NOSTOS_OK / NOSTOS_ERROR.
 ***********************************************/
static int loadIslandRoutesFiltered(const char *psConfigPath, tIslandConfig *pstConfig) {
    tRouteList stRawRoutes;
    int nFilterResult = 0;

    if (NOSTOS_OK != loadIslandConfig(psConfigPath, pstConfig, &stRawRoutes)) {
        return NOSTOS_ERROR;
    }
    nFilterResult = filterIslandRoutes(pstConfig->psName, &stRawRoutes, &pstConfig->stRoutes);
    destroyRouteList(&stRawRoutes);
    if (0 > nFilterResult) {
        destroyIslandConfig(pstConfig);
        return NOSTOS_ERROR;
    }
    return NOSTOS_OK;
}

/***********************************************
 * @Name: initializeIsland
 * @Def: Blocks SIGINT, creates its signalfd, loads and filters the
 *       island configuration, then loads its binary stock, in that
 *       order. Each stage releases only what it itself acquired.
 * @Arg: In: psConfigPath, psStockPath.
 *       Out: pnSigFd, pstConfig, pstStock.
 * @Ret: NOSTOS_OK / NOSTOS_ERROR.
 ***********************************************/
static int initializeIsland(const char *psConfigPath, const char *psStockPath, int *pnSigFd,
                             tIslandConfig *pstConfig, tStockList *pstStock) {
    sigset_t stOldMask;

    if (NOSTOS_OK != blockSigint(&stOldMask)) {
        return NOSTOS_ERROR;
    }
    *pnSigFd = createSigintFd();
    if (-1 == *pnSigFd) {
        return NOSTOS_ERROR;
    }
    if (NOSTOS_OK != loadIslandRoutesFiltered(psConfigPath, pstConfig)) {
        close(*pnSigFd);
        return NOSTOS_ERROR;
    }
    if (NOSTOS_OK != loadStockList(psStockPath, pstStock)) {
        destroyIslandConfig(pstConfig);
        close(*pnSigFd);
        return NOSTOS_ERROR;
    }
    return NOSTOS_OK;
}

/***********************************************
 * @Name: announceIslandReady
 * @Def: Prints the required startup messages with the actual loaded
 *       capacity/route/product counts (never a hardcoded fixture
 *       count).
 * @Arg: In: pstConfig, pstStock.
 * @Ret: NOSTOS_OK / NOSTOS_ERROR.
 ***********************************************/
static int announceIslandReady(const tIslandConfig *pstConfig, const tStockList *pstStock) {
    if (NOSTOS_OK != writeFormatted(STDOUT_FILENO, "Island %s initialized.\n", pstConfig->psName)) {
        return NOSTOS_ERROR;
    }
    if (NOSTOS_OK != writeFormatted(STDOUT_FILENO, "Port capacity: %d ships.\n", pstConfig->nCapacity)) {
        return NOSTOS_ERROR;
    }
    if (NOSTOS_OK != writeFormatted(STDOUT_FILENO, "%d sea routes loaded.\n", pstConfig->stRoutes.nCount)) {
        return NOSTOS_ERROR;
    }
    return writeFormatted(STDOUT_FILENO, "%d products available.\n", pstStock->nCount);
}

/***********************************************
 * @Name: runIslandLifecycle
 * @Def: Announces readiness, blocks until CTRL+C, prints the shutdown
 *       message, and releases every owned resource exactly once,
 *       regardless of which stage failed.
 * @Arg: In: nSigFd. In/Out: pstConfig, pstStock.
 * @Ret: NOSTOS_OK / NOSTOS_ERROR.
 ***********************************************/
static int runIslandLifecycle(int nSigFd, tIslandConfig *pstConfig, tStockList *pstStock) {
    int nStatus = NOSTOS_OK;

    if (NOSTOS_OK != announceIslandReady(pstConfig, pstStock)) {
        nStatus = NOSTOS_ERROR;
    } else if (NOSTOS_OK != waitForSignalOnly(nSigFd)) {
        nStatus = NOSTOS_ERROR;
    } else {
        writeFormatted(STDOUT_FILENO, "%s closes its port.\n", pstConfig->psName);
    }
    destroyStockList(pstStock);
    destroyIslandConfig(pstConfig);
    close(nSigFd);
    return nStatus;
}

/***********************************************
 * @Name: main
 * @Def: Entry point. Validates the CLI argument count, initializes,
 *       runs the lifecycle, and returns the documented exit status
 *       (assumption A21).
 * @Arg: In: argc = argument count.
 *       In: argv[1] = config.dat path, argv[2] = stock.db path.
 * @Ret: NOSTOS_EXIT_OK / NOSTOS_EXIT_ARGS / NOSTOS_EXIT_IO.
 ***********************************************/
int main(int argc, char *argv[]) {
    int nSigFd = -1;
    tIslandConfig stConfig;
    tStockList stStock;

    if (3 != argc) {
        writeString(STDERR_FILENO, "Usage: island <config.dat> <stock.db>\n");
        return NOSTOS_EXIT_ARGS;
    }
    if (NOSTOS_OK != initializeIsland(argv[1], argv[2], &nSigFd, &stConfig, &stStock)) {
        writeString(STDERR_FILENO, "Error: Island failed to initialize.\n");
        return NOSTOS_EXIT_IO;
    }
    if (NOSTOS_OK != runIslandLifecycle(nSigFd, &stConfig, &stStock)) {
        return NOSTOS_EXIT_IO;
    }
    return NOSTOS_EXIT_OK;
}

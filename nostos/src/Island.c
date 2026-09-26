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
 * @Date: 2026-09-26
 */

#include "Island.h"

/***********************************************
 * @Name: loadIslandRoutesFiltered
 * @Def: Loads the raw configuration/routes, then mandatorily filters
 *       the routes through the real Sphragis adapter before they are
 *       ever considered valid. Self-cleaning: on any failure the
 *       island configuration is fully released before returning.
 * @Arg: In: psConfigPath = path to the island configuration.
 *       Out: pstConfig = loaded configuration; its stRoutes becomes the
 *            valid, post-filter list on success.
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR on load or Sphragis
 *       failure (configuration already released).
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
 * @Arg: In: psConfigPath = path to the island configuration.
 *       In: psStockPath = path to the binary stock file.
 *       Out: pnSigFd = open signalfd on success.
 *       Out: pstConfig = loaded, filtered configuration on success.
 *       Out: pstStock = loaded stock list on success.
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR otherwise (nothing is left
 *       allocated or open).
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
        *pnSigFd = -1;
        return NOSTOS_ERROR;
    }
    if (NOSTOS_OK != loadStockList(psStockPath, pstStock)) {
        destroyIslandConfig(pstConfig);
        close(*pnSigFd);
        *pnSigFd = -1;
        return NOSTOS_ERROR;
    }
    return NOSTOS_OK;
}

/***********************************************
 * @Name: announceIslandReady
 * @Def: Prints the required startup messages with the actual loaded
 *       capacity/route/product counts (never a hardcoded fixture
 *       count).
 * @Arg: In: pstConfig = loaded configuration (name, capacity, routes).
 *       In: pstStock = loaded stock list (product count).
 * @Ret: NOSTOS_OK if every line was written, NOSTOS_ERROR otherwise.
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
 *       regardless of which stage failed. Each failure is reported with
 *       one fixed stderr literal (no allocation, no retry).
 * @Arg: In: nSigFd = signalfd created during initialization; closed.
 *       In/Out: pstConfig = configuration (with valid routes) to release.
 *       In/Out: pstStock = stock list to release.
 * @Ret: NOSTOS_OK after a clean CTRL+C shutdown, NOSTOS_ERROR if a
 *       message could not be written or the signal wait failed.
 ***********************************************/
static int runIslandLifecycle(int nSigFd, tIslandConfig *pstConfig, tStockList *pstStock) {
    int nStatus = NOSTOS_ERROR;

    if (NOSTOS_OK != announceIslandReady(pstConfig, pstStock)) {
        (void) writeString(STDERR_FILENO, ERROR_ISLAND_WRITE);
    } else if (NOSTOS_OK != waitForSignalOnly(nSigFd)) {
        (void) writeString(STDERR_FILENO, ERROR_ISLAND_SIGNAL);
    } else if (NOSTOS_OK != writeFormatted(STDOUT_FILENO, "%s closes its port.\n", pstConfig->psName)) {
        (void) writeString(STDERR_FILENO, ERROR_ISLAND_WRITE);
    } else {
        nStatus = NOSTOS_OK;
    }
    destroyStockList(pstStock);
    destroyIslandConfig(pstConfig);
    close(nSigFd);
    return nStatus;
}

/***********************************************
 * @Name: main
 * @Def: Entry point. Validates the CLI argument count, initializes,
 *       runs the lifecycle, and returns the exit status
 * @Arg: In: argc = argument count.
 *       In: argv[1] = config.dat path, argv[2] = stock.db path.
 * @Ret: NOSTOS_EXIT_OK after a clean CTRL+C shutdown, NOSTOS_EXIT_ARGS on
 *       a wrong argument count, NOSTOS_EXIT_IO on any initialization,
 *       I/O, or allocation failure.
 ***********************************************/
int main(int argc, char *argv[]) {
    int nSigFd = -1;
    tIslandConfig stConfig;
    tStockList stStock;

    if (3 != argc) {
        (void) writeString(STDERR_FILENO, "Usage: island <config.dat> <stock.db>\n");
        return NOSTOS_EXIT_ARGS;
    }
    if (NOSTOS_OK != initializeIsland(argv[1], argv[2], &nSigFd, &stConfig, &stStock)) {
        (void) writeString(STDERR_FILENO, "Error: Island failed to initialize.\n");
        return NOSTOS_EXIT_IO;
    }
    if (NOSTOS_OK != runIslandLifecycle(nSigFd, &stConfig, &stStock)) {
        return NOSTOS_EXIT_IO;
    }
    return NOSTOS_EXIT_OK;
}

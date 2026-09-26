/*
 * @File: Ithaca.c
 * @Purpose: Entry point for the Ithaca process. Usage:
 *           ./ithaca <config.dat> <voyages.dat>
 *           Loads its configuration and voyage list, announces
 *           readiness, then blocks (no busy waiting) until CTRL+C,
 *           at which point it releases every owned resource and
 *           exits. Ithaca has no interactive terminal (P p.17).
 * @Author: Salah Ahmed Salaheldin Adly Rashwan
 * @Date: 2026-09-21
 */

#include "Ithaca.h"

/***********************************************
 * @Name: initializeIthaca
 * @Def: Blocks SIGINT, creates its signalfd, then loads Ithaca's
 *       configuration and voyage list in that order. Each stage
 *       releases only what it itself acquired on failure; loaders are
 *       self-cleaning on their own failure.
 * @Arg: In: psConfigPath, psVoyagesPath.
 *       Out: pnSigFd, pstConfig, pstVoyages.
 * @Ret: NOSTOS_OK / NOSTOS_ERROR.
 ***********************************************/
static int initializeIthaca(const char *psConfigPath, const char *psVoyagesPath, int *pnSigFd,
                             tIthacaConfig *pstConfig, tVoyageList *pstVoyages) {
    sigset_t stOldMask;

    if (NOSTOS_OK != blockSigint(&stOldMask)) {
        return NOSTOS_ERROR;
    }
    *pnSigFd = createSigintFd();
    if (-1 == *pnSigFd) {
        return NOSTOS_ERROR;
    }
    if (NOSTOS_OK != loadIthacaConfig(psConfigPath, pstConfig)) {
        close(*pnSigFd);
        return NOSTOS_ERROR;
    }
    if (NOSTOS_OK != loadVoyages(psVoyagesPath, pstVoyages)) {
        destroyIthacaConfig(pstConfig);
        close(*pnSigFd);
        return NOSTOS_ERROR;
    }
    return NOSTOS_OK;
}

/***********************************************
 * @Name: announceIthacaReady
 * @Def: Prints the required startup messages with the actual loaded
 *       voyage count (never a hardcoded fixture count).
 * @Arg: In: pstVoyages.
 * @Ret: NOSTOS_OK / NOSTOS_ERROR.
 ***********************************************/
static int announceIthacaReady(const tVoyageList *pstVoyages) {
    if (NOSTOS_OK != writeFormatted(STDOUT_FILENO, "Ithaca initialized. %d voyages loaded.\n", pstVoyages->nCount)) {
        return NOSTOS_ERROR;
    }
    return writeString(STDOUT_FILENO, "Waiting for Odysseus...\n");
}

/***********************************************
 * @Name: runIthacaLifecycle
 * @Def: Announces readiness, blocks until CTRL+C, prints the shutdown
 *       message, and releases every owned resource exactly once,
 *       regardless of which stage failed.
 * @Arg: In: nSigFd. In/Out: pstConfig, pstVoyages.
 * @Ret: NOSTOS_OK / NOSTOS_ERROR.
 ***********************************************/
static int runIthacaLifecycle(int nSigFd, tIthacaConfig *pstConfig, tVoyageList *pstVoyages) {
    int nStatus = NOSTOS_OK;

    if (NOSTOS_OK != announceIthacaReady(pstVoyages)) {
        nStatus = NOSTOS_ERROR;
    } else if (NOSTOS_OK != waitForSignalOnly(nSigFd)) {
        nStatus = NOSTOS_ERROR;
    } else {
        writeString(STDOUT_FILENO, "Ithaca closes the harbor.\n");
    }
    destroyVoyageList(pstVoyages);
    destroyIthacaConfig(pstConfig);
    close(nSigFd);
    return nStatus;
}

/***********************************************
 * @Name: main
 * @Def: Entry point. Validates the CLI argument count, initializes,
 *       runs the lifecycle, and returns the documented exit status
 *       (assumption A21).
 * @Arg: In: argc = argument count.
 *       In: argv[1] = config.dat path, argv[2] = voyages.dat path.
 * @Ret: NOSTOS_EXIT_OK / NOSTOS_EXIT_ARGS / NOSTOS_EXIT_IO.
 ***********************************************/
int main(int argc, char *argv[]) {
    int nSigFd = -1;
    tIthacaConfig stConfig;
    tVoyageList stVoyages;

    if (3 != argc) {
        writeString(STDERR_FILENO, "Usage: ithaca <config.dat> <voyages.dat>\n");
        return NOSTOS_EXIT_ARGS;
    }
    if (NOSTOS_OK != initializeIthaca(argv[1], argv[2], &nSigFd, &stConfig, &stVoyages)) {
        writeString(STDERR_FILENO, "Error: Ithaca failed to initialize.\n");
        return NOSTOS_EXIT_IO;
    }
    if (NOSTOS_OK != runIthacaLifecycle(nSigFd, &stConfig, &stVoyages)) {
        return NOSTOS_EXIT_IO;
    }
    return NOSTOS_EXIT_OK;
}

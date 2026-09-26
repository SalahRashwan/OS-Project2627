/*
 * @File: voyages.c
 * @Purpose: Loader and destructor for Ithaca's voyages.dat file.
 * @Author: Salah Ahmed Salaheldin Adly Rashwan
 * @Date: 2026-09-21
 */

/* System */
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>

/* Own */
#include "voyages.h"
#include "text.h"
#include "io.h"
#include "status.h"

/***********************************************
 * @Name: initVoyageList
 * @Def: Resets a voyage list to a safe, destroyable empty state.
 * @Arg: Out: pstList.
 * @Ret: None.
 ***********************************************/
static void initVoyageList(tVoyageList *pstList) {
    pstList->pstVoyages = NULL;
    pstList->nCount = 0;
    pstList->nCapacity = 0;
}

/***********************************************
 * @Name: destroyVoyageList
 * @Def: See voyages.h.
 ***********************************************/
void destroyVoyageList(tVoyageList *pstList) {
    int nIndex = 0;

    if (NULL != pstList->pstVoyages) {
        for (nIndex = 0; nIndex < pstList->nCount; nIndex++) {
            free(pstList->pstVoyages[nIndex].psObject);
            free(pstList->pstVoyages[nIndex].psFilePath);
            free(pstList->pstVoyages[nIndex].psDestination);
        }
        free(pstList->pstVoyages);
    }
    initVoyageList(pstList);
}

/***********************************************
 * @Name: voyageListAppend
 * @Def: Appends one already-owned voyage to a growable voyage list.
 * @Arg: In/Out: pstList = list to grow.
 *       In: pstVoyage = voyage to move into the list on success.
 * @Ret: NOSTOS_OK / NOSTOS_ERROR.
 ***********************************************/
static int voyageListAppend(tVoyageList *pstList, const tVoyage *pstVoyage) {
    tVoyage *pTemp = NULL;
    int nNewCapacity = 0;

    if (pstList->nCount == pstList->nCapacity) {
        if (0 == pstList->nCapacity) {
            nNewCapacity = 8;
        } else {
            nNewCapacity = pstList->nCapacity * 2;
        }
        pTemp = realloc(pstList->pstVoyages, (size_t) nNewCapacity * sizeof(tVoyage));
        if (NULL == pTemp) {
            return NOSTOS_ERROR;
        }
        pstList->pstVoyages = pTemp;
        pstList->nCapacity = nNewCapacity;
    }
    pstList->pstVoyages[pstList->nCount] = *pstVoyage;
    pstList->nCount++;
    return NOSTOS_OK;
}

/***********************************************
 * @Name: parseVoyageLine
 * @Def: Parses a four-token "<OBJECT> <FILE> <DESTINATION> <REWARD>"
 *       line into an owned, still-unnumbered voyage record.
 * @Arg: In/Out: psLine = mutable line to tokenize.
 *       In: nNextId = identifier to assign to this record.
 *       Out: pstVoyage = filled on success.
 * @Ret: NOSTOS_OK / NOSTOS_ERROR.
 ***********************************************/
static int parseVoyageLine(char *psLine, int nNextId, tVoyage *pstVoyage) {
    char **appsTokens = NULL;
    int nTokenCount = 0;
    long lReward = 0;

    if (NOSTOS_OK != tokenizeLine(psLine, &appsTokens, &nTokenCount)) {
        return NOSTOS_ERROR;
    }
    if (4 != nTokenCount || NOSTOS_OK != parseDigitsToLong(appsTokens[3], &lReward) || lReward > INT_MAX) {
        free(appsTokens);
        return NOSTOS_ERROR;
    }
    pstVoyage->nId = nNextId;
    pstVoyage->psObject = duplicateString(appsTokens[0]);
    pstVoyage->psFilePath = duplicateString(appsTokens[1]);
    pstVoyage->psDestination = duplicateString(appsTokens[2]);
    pstVoyage->nReward = (int) lReward;
    free(appsTokens);
    if (NULL == pstVoyage->psObject || NULL == pstVoyage->psFilePath || NULL == pstVoyage->psDestination) {
        free(pstVoyage->psObject);
        free(pstVoyage->psFilePath);
        free(pstVoyage->psDestination);
        return NOSTOS_ERROR;
    }
    return NOSTOS_OK;
}

/***********************************************
 * @Name: failVoyages
 * @Def: Single cleanup path for a failed voyages.dat load.
 * @Arg: In: nFd, In/Out: pstBuffer, pstList.
 * @Ret: NOSTOS_ERROR, always.
 ***********************************************/
static int failVoyages(int nFd, tLineBuffer *pstBuffer, tVoyageList *pstList) {
    close(nFd);
    lineBufferDestroy(pstBuffer);
    destroyVoyageList(pstList);
    return NOSTOS_ERROR;
}

/***********************************************
 * @Name: loadVoyageRecords
 * @Def: Reads every voyage line until EOF, skipping blank lines and
 *       assigning sequential internal identifiers in file order.
 * @Arg: In: nFd, In/Out: pstBuffer, Out: pstList.
 * @Ret: NOSTOS_OK / NOSTOS_ERROR.
 ***********************************************/
static int loadVoyageRecords(int nFd, tLineBuffer *pstBuffer, tVoyageList *pstList) {
    char *psLine = NULL;
    tVoyage stVoyage;
    int nReadStatus = 0;

    for (;;) {
        nReadStatus = readNextLine(nFd, pstBuffer, &psLine);
        if (LINE_EOF == nReadStatus) {
            return NOSTOS_OK;
        }
        if (LINE_FOUND != nReadStatus) {
            return NOSTOS_ERROR;
        }
        if ('\0' == psLine[0]) {
            free(psLine);
            continue;
        }
        if (NOSTOS_OK != parseVoyageLine(psLine, pstList->nCount + 1, &stVoyage)) {
            free(psLine);
            return NOSTOS_ERROR;
        }
        free(psLine);
        if (NOSTOS_OK != voyageListAppend(pstList, &stVoyage)) {
            free(stVoyage.psObject);
            free(stVoyage.psFilePath);
            free(stVoyage.psDestination);
            return NOSTOS_ERROR;
        }
    }
}

/***********************************************
 * @Name: loadVoyages
 * @Def: See voyages.h.
 ***********************************************/
int loadVoyages(const char *psPath, tVoyageList *pstList) {
    int nFd = -1;
    tLineBuffer stBuffer;

    initVoyageList(pstList);
    lineBufferInit(&stBuffer);
    nFd = safeOpenReadOnly(psPath);
    if (-1 == nFd) {
        return NOSTOS_ERROR;
    }
    if (NOSTOS_OK != loadVoyageRecords(nFd, &stBuffer, pstList)) {
        return failVoyages(nFd, &stBuffer, pstList);
    }
    close(nFd);
    lineBufferDestroy(&stBuffer);
    return NOSTOS_OK;
}

/*
 * @File: text.c
 * @Purpose: Line accumulation, whitespace tokenization, digit-only
 *           numeric parsing, and owned-string duplication shared by
 *           every loader and by the Odysseus terminal loop.
 * @Author: Salah Ahmed Salaheldin Adly Rashwan
 * @Date: 2026-09-21
 */

/* System */
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

/* Own */
#include "text.h"
#include "status.h"
#include "io.h"

/* Initial/growth sizing for both the byte buffer and the token array. */
#define LINE_BUFFER_INITIAL_CAPACITY 64
#define TOKEN_ARRAY_INITIAL_CAPACITY 4
/* Chunk size used to fill the accumulator from a regular file. */
#define READ_CHUNK_SIZE 512

/***********************************************
 * @Name: lineBufferInit
 * @Def: See text.h.
 ***********************************************/
void lineBufferInit(tLineBuffer *pstBuffer) {
    pstBuffer->psData = NULL;
    pstBuffer->nLength = 0;
    pstBuffer->nCapacity = 0;
}

/***********************************************
 * @Name: lineBufferDestroy
 * @Def: See text.h.
 ***********************************************/
void lineBufferDestroy(tLineBuffer *pstBuffer) {
    free(pstBuffer->psData);
    pstBuffer->psData = NULL;
    pstBuffer->nLength = 0;
    pstBuffer->nCapacity = 0;
}

/***********************************************
 * @Name: lineBufferGrow
 * @Def: Ensures capacity for at least nNeeded bytes, doubling from a
 *       small initial size. Private helper, not declared in text.h.
 * @Arg: In/Out: pstBuffer = buffer to grow.
 *       In: nNeeded = minimum total capacity required.
 * @Ret: NOSTOS_OK / NOSTOS_ERROR.
 ***********************************************/
static int lineBufferGrow(tLineBuffer *pstBuffer, size_t nNeeded) {
    size_t nNewCapacity = 0;
    char *pTemp = NULL;

    if (nNeeded <= pstBuffer->nCapacity) {
        return NOSTOS_OK;
    }
    if (0 == pstBuffer->nCapacity) {
        nNewCapacity = LINE_BUFFER_INITIAL_CAPACITY;
    } else {
        nNewCapacity = pstBuffer->nCapacity;
    }
    while (nNewCapacity < nNeeded) {
        nNewCapacity *= 2;
    }
    pTemp = realloc(pstBuffer->psData, nNewCapacity);
    if (NULL == pTemp) {
        return NOSTOS_ERROR;
    }
    pstBuffer->psData = pTemp;
    pstBuffer->nCapacity = nNewCapacity;
    return NOSTOS_OK;
}

/***********************************************
 * @Name: lineBufferAppend
 * @Def: See text.h.
 ***********************************************/
int lineBufferAppend(tLineBuffer *pstBuffer, const char *pData, size_t nLen) {
    if (NOSTOS_OK != lineBufferGrow(pstBuffer, pstBuffer->nLength + nLen)) {
        return NOSTOS_ERROR;
    }
    memcpy(pstBuffer->psData + pstBuffer->nLength, pData, nLen);
    pstBuffer->nLength += nLen;
    return NOSTOS_OK;
}

/***********************************************
 * @Name: makeOwnedLine
 * @Def: Duplicates nLen raw bytes into a newly owned NUL-terminated
 *       string. Private helper, not declared in text.h.
 * @Arg: In: pData = source bytes.
 *       In: nLen = number of bytes.
 *       Out: ppsLine = set to the owned copy on success.
 * @Ret: NOSTOS_OK / NOSTOS_ERROR.
 ***********************************************/
static int makeOwnedLine(const char *pData, size_t nLen, char **ppsLine) {
    char *psLine = malloc(nLen + 1);

    if (NULL == psLine) {
        return NOSTOS_ERROR;
    }
    memcpy(psLine, pData, nLen);
    psLine[nLen] = '\0';
    *ppsLine = psLine;
    return NOSTOS_OK;
}

/***********************************************
 * @Name: lineBufferExtractLine
 * @Def: See text.h.
 ***********************************************/
int lineBufferExtractLine(tLineBuffer *pstBuffer, char **ppsLine) {
    size_t nIndex = 0;
    size_t nEffectiveLen = 0;
    size_t nRemaining = 0;

    for (nIndex = 0; nIndex < pstBuffer->nLength; nIndex++) {
        if ('\n' == pstBuffer->psData[nIndex]) {
            nEffectiveLen = nIndex;
            if (0 < nEffectiveLen && '\r' == pstBuffer->psData[nEffectiveLen - 1]) {
                nEffectiveLen--;
            }
            if (NOSTOS_OK != makeOwnedLine(pstBuffer->psData, nEffectiveLen, ppsLine)) {
                return NOSTOS_ERROR;
            }
            nRemaining = pstBuffer->nLength - (nIndex + 1);
            memmove(pstBuffer->psData, pstBuffer->psData + nIndex + 1, nRemaining);
            pstBuffer->nLength = nRemaining;
            return LINE_FOUND;
        }
    }
    return LINE_NONE;
}

/***********************************************
 * @Name: lineBufferTakeRemainder
 * @Def: See text.h.
 ***********************************************/
int lineBufferTakeRemainder(tLineBuffer *pstBuffer, char **ppsLine) {
    int nStatus = NOSTOS_ERROR;

    if (0 == pstBuffer->nLength) {
        return LINE_NONE;
    }
    nStatus = makeOwnedLine(pstBuffer->psData, pstBuffer->nLength, ppsLine);
    if (NOSTOS_OK != nStatus) {
        return NOSTOS_ERROR;
    }
    pstBuffer->nLength = 0;
    return LINE_FOUND;
}

/***********************************************
 * @Name: readNextLine
 * @Def: See text.h.
 ***********************************************/
int readNextLine(int nFd, tLineBuffer *pstBuffer, char **ppsLine) {
    char acChunk[READ_CHUNK_SIZE];
    ssize_t nRead = 0;
    int nExtracted = 0;

    for (;;) {
        nExtracted = lineBufferExtractLine(pstBuffer, ppsLine);
        if (NOSTOS_ERROR == nExtracted) {
            return NOSTOS_ERROR;
        }
        if (LINE_FOUND == nExtracted) {
            return LINE_FOUND;
        }
        nRead = safeRead(nFd, acChunk, sizeof(acChunk));
        if (0 > nRead) {
            return NOSTOS_ERROR;
        }
        if (0 == nRead) {
            nExtracted = lineBufferTakeRemainder(pstBuffer, ppsLine);
            if (NOSTOS_ERROR == nExtracted) {
                return NOSTOS_ERROR;
            }
            if (LINE_FOUND == nExtracted) {
                return LINE_FOUND;
            }
            return LINE_EOF;
        }
        if (NOSTOS_OK != lineBufferAppend(pstBuffer, acChunk, (size_t) nRead)) {
            return NOSTOS_ERROR;
        }
    }
}

/***********************************************
 * @Name: tokenizeLine
 * @Def: See text.h.
 ***********************************************/
int tokenizeLine(char *psLine, char ***pappsTokens, int *pnTokenCount) {
    char **appsTokens = NULL;
    size_t nCapacity = 0;
    int nCount = 0;
    char *psSavePtr = NULL;
    char *psToken = NULL;
    char **pTemp = NULL;

    psToken = strtok_r(psLine, " \t", &psSavePtr);
    while (NULL != psToken) {
        if ((size_t) nCount == nCapacity) {
            if (0 == nCapacity) {
                nCapacity = TOKEN_ARRAY_INITIAL_CAPACITY;
            } else {
                nCapacity = nCapacity * 2;
            }
            pTemp = realloc(appsTokens, nCapacity * sizeof(char *));
            if (NULL == pTemp) {
                free(appsTokens);
                return NOSTOS_ERROR;
            }
            appsTokens = pTemp;
        }
        appsTokens[nCount] = psToken;
        nCount++;
        psToken = strtok_r(NULL, " \t", &psSavePtr);
    }
    *pappsTokens = appsTokens;
    *pnTokenCount = nCount;
    return NOSTOS_OK;
}

/***********************************************
 * @Name: parseDigitsToLong
 * @Def: See text.h.
 ***********************************************/
int parseDigitsToLong(const char *psToken, long *plValue) {
    const char *psCursor = NULL;
    char *psEnd = NULL;
    long lValue = 0;

    if (NULL == psToken || '\0' == psToken[0]) {
        return NOSTOS_ERROR;
    }
    for (psCursor = psToken; '\0' != *psCursor; psCursor++) {
        if (0 == isdigit((unsigned char) *psCursor)) {
            return NOSTOS_ERROR;
        }
    }
    errno = 0;
    lValue = strtol(psToken, &psEnd, 10);
    if ('\0' != *psEnd || psEnd == psToken) {
        return NOSTOS_ERROR;
    }
    if (ERANGE == errno) {
        return NOSTOS_ERROR;
    }
    *plValue = lValue;
    return NOSTOS_OK;
}

/***********************************************
 * @Name: duplicateString
 * @Def: See text.h.
 ***********************************************/
char *duplicateString(const char *psSource) {
    size_t nLen = 0;
    char *psCopy = NULL;

    if (NULL == psSource) {
        return NULL;
    }
    nLen = strlen(psSource);
    psCopy = malloc(nLen + 1);
    if (NULL == psCopy) {
        return NULL;
    }
    memcpy(psCopy, psSource, nLen + 1);
    return psCopy;
}

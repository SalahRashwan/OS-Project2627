/*
 * @File: text.c
 * @Purpose: Line accumulation, whitespace tokenization, digit-only
 *           numeric parsing, and owned-string duplication shared by
 *           every loader and by the Odysseus terminal loop.
 * @Author: Salah Ahmed Salaheldin Adly Rashwan
 * @Date: 2026-09-26
 */

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
 * @Def: Initializes an empty line buffer, safe to destroy immediately.
 * @Arg: Out: pstBuffer = buffer to initialize.
 * @Ret: None.
 ***********************************************/
void lineBufferInit(tLineBuffer *pstBuffer) {
    pstBuffer->psData = NULL;
    pstBuffer->nLength = 0;
    pstBuffer->nCapacity = 0;
}

/***********************************************
 * @Name: lineBufferDestroy
 * @Def: Frees the owned storage of a line buffer and resets its fields.
 * @Arg: In/Out: pstBuffer = buffer to release; safe on a zeroed buffer.
 * @Ret: None.
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
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR on failure.
 ***********************************************/
static int lineBufferGrow(tLineBuffer *pstBuffer, size_t nNeeded) {
    size_t nNewCapacity = 0;
    char *psTemp = NULL;

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
    psTemp = realloc(pstBuffer->psData, nNewCapacity);
    if (NULL == psTemp) {
        return NOSTOS_ERROR;
    }
    pstBuffer->psData = psTemp;
    pstBuffer->nCapacity = nNewCapacity;
    return NOSTOS_OK;
}

/***********************************************
 * @Name: lineBufferAppend
 * @Def: Appends raw bytes to the buffer, growing storage geometrically.
 * @Arg: In/Out: pstBuffer = buffer to grow.
 *       In: pData = bytes to append.
 *       In: nLen = number of bytes to append.
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR on allocation failure (the
 *       buffer's previous contents remain valid and owned).
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
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR on failure.
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
 * @Def: Removes and returns one complete newline-terminated line from
 *       the front of the buffer, normalizing a trailing CRLF to nothing.
 * @Arg: In/Out: pstBuffer = buffer to consume from.
 *       Out: ppsLine = set to a newly owned, NUL-terminated line on
 *            LINE_FOUND; left untouched otherwise.
 * @Ret: LINE_FOUND, LINE_NONE, or NOSTOS_ERROR on allocation failure.
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
 * @Def: Takes whatever bytes remain (no trailing newline) as one final
 *       line; intended to run exactly once after EOF.
 * @Arg: In/Out: pstBuffer = buffer to drain.
 *       Out: ppsLine = set to a newly owned, NUL-terminated line on
 *            LINE_FOUND; left untouched otherwise.
 * @Ret: LINE_FOUND, LINE_NONE (buffer already empty), or NOSTOS_ERROR.
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
 * @Def: Reads and returns the next complete line from a descriptor,
 *       accumulating raw chunks into pstBuffer across calls. Adapts the
 *       delimiter-reading concept taught in Sockets/client.c into a
 *       safe, bounded-growth, EOF/error-distinguishing form.
 * @Arg: In: nFd = descriptor to read from (a regular file).
 *       In/Out: pstBuffer = accumulator buffer, reused across calls.
 *       Out: ppsLine = set to a newly owned line on LINE_FOUND.
 * @Ret: LINE_FOUND (a line, possibly the final unterminated one, was
 *       produced), LINE_EOF (clean end of file, nothing left), or
 *       NOSTOS_ERROR on a read or allocation failure.
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
 * @Def: Splits a mutable line in place on spaces/tabs using strtok_r.
 * @Arg: In/Out: psLine = line to tokenize; modified with embedded NULs.
 *       Out: pappsTokens = set to a newly owned array of pointers that
 *            borrow into psLine (do not outlive it; duplicate what must
 *            survive).
 *       Out: pnTokenCount = number of tokens found (may be 0).
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR on allocation failure.
 ***********************************************/
int tokenizeLine(char *psLine, char ***pappsTokens, int *pnTokenCount) {
    char **appsTokens = NULL;
    size_t nCapacity = 0;
    int nCount = 0;
    char *psSavePtr = NULL;
    char *psToken = NULL;
    char **appsTemp = NULL;

    psToken = strtok_r(psLine, " \t", &psSavePtr);
    while (NULL != psToken) {
        if ((size_t) nCount == nCapacity) {
            if (0 == nCapacity) {
                nCapacity = TOKEN_ARRAY_INITIAL_CAPACITY;
            } else {
                nCapacity = nCapacity * 2;
            }
            appsTemp = realloc(appsTokens, nCapacity * sizeof(char *));
            if (NULL == appsTemp) {
                free(appsTokens);
                return NOSTOS_ERROR;
            }
            appsTokens = appsTemp;
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
 * @Def: Parses a token as an unsigned, digit-only decimal integer.
 *       Signs, decimal points, exponents, leading/trailing junk, empty
 *       tokens, and out-of-range values are all rejected.
 * @Arg: In: psToken = token to parse.
 *       Out: plValue = parsed value, set only on success.
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR otherwise.
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
 * @Def: Allocates and returns an owned copy of a NUL-terminated string.
 * @Arg: In: psSource = string to copy; must not be NULL.
 * @Ret: A newly owned copy, or NULL on allocation failure.
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

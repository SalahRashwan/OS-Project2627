#ifndef NOSTOS_TEXT_H
#define NOSTOS_TEXT_H

/*
 * @File: text.h
 * @Purpose: Line accumulation over raw descriptor reads, whitespace
 *           tokenization, digit-only numeric parsing, and owned-string
 *           duplication. Shared by every config/data loader and by the
 *           Odysseus terminal loop.
 * @Author: Salah Ahmed Salaheldin Adly Rashwan
 * @Date: 2026-09-21
 */

/* System Includes */
#include <stddef.h>

/* A complete line was extracted (or a nonempty remainder was taken). */
#define LINE_FOUND 1
/* No complete line is available yet; more data is needed. */
#define LINE_NONE 0

/* Owned, growable raw-byte buffer used to accumulate descriptor reads
 * until complete lines can be extracted from them. */
typedef struct {
    char *psData;
    size_t nLength;
    size_t nCapacity;
} tLineBuffer;

/***********************************************
 * @Name: lineBufferInit
 * @Def: Initializes an empty line buffer, safe to destroy immediately.
 * @Arg: Out: pstBuffer = buffer to initialize.
 * @Ret: None.
 ***********************************************/
void lineBufferInit(tLineBuffer *pstBuffer);

/***********************************************
 * @Name: lineBufferDestroy
 * @Def: Frees the owned storage of a line buffer and resets its fields.
 * @Arg: In/Out: pstBuffer = buffer to release; safe on a zeroed buffer.
 * @Ret: None.
 ***********************************************/
void lineBufferDestroy(tLineBuffer *pstBuffer);

/***********************************************
 * @Name: lineBufferAppend
 * @Def: Appends raw bytes to the buffer, growing storage geometrically.
 * @Arg: In/Out: pstBuffer = buffer to grow.
 *       In: pData = bytes to append.
 *       In: nLen = number of bytes to append.
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR on allocation failure (the
 *       buffer's previous contents remain valid and owned).
 ***********************************************/
int lineBufferAppend(tLineBuffer *pstBuffer, const char *pData, size_t nLen);

/***********************************************
 * @Name: lineBufferExtractLine
 * @Def: Removes and returns one complete newline-terminated line from
 *       the front of the buffer, normalizing a trailing CRLF to nothing.
 * @Arg: In/Out: pstBuffer = buffer to consume from.
 *       Out: ppsLine = set to a newly owned, NUL-terminated line on
 *            LINE_FOUND; left untouched otherwise.
 * @Ret: LINE_FOUND, LINE_NONE, or NOSTOS_ERROR on allocation failure.
 ***********************************************/
int lineBufferExtractLine(tLineBuffer *pstBuffer, char **ppsLine);

/***********************************************
 * @Name: lineBufferTakeRemainder
 * @Def: Takes whatever bytes remain (no trailing newline) as one final
 *       line; intended to run exactly once after EOF.
 * @Arg: In/Out: pstBuffer = buffer to drain.
 *       Out: ppsLine = set to a newly owned, NUL-terminated line on
 *            LINE_FOUND; left untouched otherwise.
 * @Ret: LINE_FOUND, LINE_NONE (buffer already empty), or NOSTOS_ERROR.
 ***********************************************/
int lineBufferTakeRemainder(tLineBuffer *pstBuffer, char **ppsLine);

/* readNextLine() outcomes beyond LINE_FOUND/NOSTOS_ERROR. */
#define LINE_EOF 2

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
int readNextLine(int nFd, tLineBuffer *pstBuffer, char **ppsLine);

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
int tokenizeLine(char *psLine, char ***pappsTokens, int *pnTokenCount);

/***********************************************
 * @Name: parseDigitsToLong
 * @Def: Parses a token as an unsigned, digit-only decimal integer.
 *       Signs, decimal points, exponents, leading/trailing junk, empty
 *       tokens, and out-of-range values are all rejected.
 * @Arg: In: psToken = token to parse.
 *       Out: plValue = parsed value, set only on success.
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR otherwise.
 ***********************************************/
int parseDigitsToLong(const char *psToken, long *plValue);

/***********************************************
 * @Name: duplicateString
 * @Def: Allocates and returns an owned copy of a NUL-terminated string.
 * @Arg: In: psSource = string to copy; must not be NULL.
 * @Ret: A newly owned copy, or NULL on allocation failure.
 ***********************************************/
char *duplicateString(const char *psSource);

#endif

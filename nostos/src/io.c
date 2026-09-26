/*
 * @File: io.c
 * @Purpose: Descriptor-only I/O helpers shared by every loader and by the
 *           Odysseus terminal: writeAll/writeString/writeFormatted for
 *           output, safeRead/safeOpenReadOnly for input, all built only
 *           on read()/write()/open()/close().
 * @Author: Salah Ahmed Salaheldin Adly Rashwan
 * @Date: 2026-09-26
 */

/* Own */
#include "io.h"
#include "status.h"

/***********************************************
 * @Name: writeAll
 * @Def: Writes every byte of a buffer to a descriptor, retrying short
 *       writes and interrupted writes as needed.
 * @Arg: In: nFd = destination descriptor.
 *       In: pBuffer = bytes to write.
 *       In: nLength = number of bytes to write.
 * @Ret: NOSTOS_OK once all bytes are written, NOSTOS_ERROR on any
 *       unrecoverable write failure (including a zero-byte write with
 *       data remaining).
 ***********************************************/
int writeAll(int nFd, const char *pBuffer, size_t nLength) {
    size_t nWritten = 0;
    ssize_t nResult = 0;

    while (nWritten < nLength) {
        nResult = write(nFd, pBuffer + nWritten, nLength - nWritten);
        if (0 > nResult) {
            if (EINTR == errno) {
                continue;
            }
            return NOSTOS_ERROR;
        }
        /* A zero-byte write with data remaining would spin forever if
         * retried blindly; treat it as a failure instead. */
        if (0 == nResult) {
            return NOSTOS_ERROR;
        }
        nWritten += (size_t) nResult;
    }
    return NOSTOS_OK;
}

/***********************************************
 * @Name: writeString
 * @Def: Writes a NUL-terminated string to a descriptor.
 * @Arg: In: nFd = destination descriptor.
 *       In: psText = NUL-terminated text to write.
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR if psText is NULL or the
 *       write failed.
 ***********************************************/
int writeString(int nFd, const char *psText) {
    if (NULL == psText) {
        return NOSTOS_ERROR;
    }
    return writeAll(nFd, psText, strlen(psText));
}

/***********************************************
 * @Name: writeFormatted
 * @Def: Formats a message in memory with vasprintf, writes it, then
 *       frees the buffer (the course's format -> write -> free idiom).
 *       Never reads or frees the output pointer when formatting failed.
 * @Arg: In: nFd = destination descriptor.
 *       In: psFormat = printf-style format string.
 *       In: ... = format arguments.
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR if formatting or writing
 *       failed.
 ***********************************************/
int writeFormatted(int nFd, const char *psFormat, ...) {
    va_list stArgs;
    char *psMessage = NULL;
    int nFormatted = 0;
    int nStatus = NOSTOS_ERROR;

    va_start(stArgs, psFormat);
    nFormatted = vasprintf(&psMessage, psFormat, stArgs);
    va_end(stArgs);

    /* A negative result means psMessage is not a usable/freeable
     * pointer; do not touch it. */
    if (0 > nFormatted) {
        return NOSTOS_ERROR;
    }
    nStatus = writeAll(nFd, psMessage, (size_t) nFormatted);
    free(psMessage);
    return nStatus;
}

/***********************************************
 * @Name: safeRead
 * @Def: Reads from a descriptor, retrying automatically on EINTR.
 * @Arg: In: nFd = source descriptor.
 *       Out: pBuffer = destination buffer.
 *       In: nCount = maximum number of bytes to read.
 * @Ret: Number of bytes read (0 means EOF), or -1 on a real error with
 *       errno set by read().
 ***********************************************/
ssize_t safeRead(int nFd, void *pBuffer, size_t nCount) {
    ssize_t nResult = 0;

    do {
        nResult = read(nFd, pBuffer, nCount);
    } while (0 > nResult && EINTR == errno);

    return nResult;
}

/***********************************************
 * @Name: safeOpenReadOnly
 * @Def: Opens a file read-only, retrying automatically on EINTR.
 * @Arg: In: psPath = path to open.
 * @Ret: An open descriptor on success, or -1 on failure with errno set.
 ***********************************************/
int safeOpenReadOnly(const char *psPath) {
    int nFd = -1;

    do {
        nFd = open(psPath, O_RDONLY);
    } while (-1 == nFd && EINTR == errno);

    return nFd;
}

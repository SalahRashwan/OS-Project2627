/*
 * @File: io.c
 * @Purpose: Descriptor-only I/O helpers shared by every loader and by the
 *           Odysseus terminal: writeAll/writeString/writeFormatted for
 *           output, safeRead/safeOpenReadOnly for input, all built only
 *           on read()/write()/open()/close().
 * @Author: Salah Ahmed Salaheldin Adly Rashwan
 * @Date: 2026-09-21
 */

/* System */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Own */
#include "io.h"
#include "status.h"

/***********************************************
 * @Name: writeAll
 * @Def: See io.h.
 * @Arg: In: nFd, pBuffer, nLength.
 * @Ret: NOSTOS_OK / NOSTOS_ERROR.
 ***********************************************/
int writeAll(int nFd, const char *pBuffer, size_t nLength) {
    size_t nWritten = 0;
    ssize_t nResult = 0;

    while (nWritten < nLength) {
        nResult = write(nFd, pBuffer + nWritten, nLength - nWritten);
        if (nResult < 0) {
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
 * @Def: See io.h.
 * @Arg: In: nFd, psText.
 * @Ret: NOSTOS_OK / NOSTOS_ERROR.
 ***********************************************/
int writeString(int nFd, const char *psText) {
    if (NULL == psText) {
        return NOSTOS_ERROR;
    }
    return writeAll(nFd, psText, strlen(psText));
}

/***********************************************
 * @Name: writeFormatted
 * @Def: See io.h.
 * @Arg: In: nFd, psFormat, ...
 * @Ret: NOSTOS_OK / NOSTOS_ERROR.
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
 * @Def: See io.h.
 * @Arg: In: nFd, pBuffer, nCount.
 * @Ret: Bytes read, 0 on EOF, -1 on real error.
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
 * @Def: See io.h.
 * @Arg: In: psPath.
 * @Ret: Open descriptor or -1.
 ***********************************************/
int safeOpenReadOnly(const char *psPath) {
    int nFd = -1;

    do {
        nFd = open(psPath, O_RDONLY);
    } while (-1 == nFd && EINTR == errno);

    return nFd;
}

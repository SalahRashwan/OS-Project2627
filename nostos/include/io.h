#ifndef NOSTOS_IO_H
#define NOSTOS_IO_H

/*
 * @File: io.h
 * @Purpose: Descriptor-only I/O helpers (read/write/open/close), the
 *           checked in-memory-format-then-write idiom, and nothing else.
 *           No stdio shortcut functions are declared or used here.
 * @Author: Salah Ahmed Salaheldin Adly Rashwan
 * @Date: 2026-09-21
 */

/* System Includes */
#include <stdarg.h>
#include <sys/types.h>

/***********************************************
 * @Name: writeAll
 * @Def: Writes every byte of a buffer to a descriptor, retrying short
 *       writes and interrupted writes as needed.
 * @Arg: In: nFd = destination descriptor.
 *       In: pBuffer = bytes to write.
 *       In: nLength = number of bytes to write.
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR on any unrecoverable write
 *       failure (including a zero-byte write with data remaining).
 ***********************************************/
int writeAll(int nFd, const char *pBuffer, size_t nLength);

/***********************************************
 * @Name: writeString
 * @Def: Writes a NUL-terminated literal/owned string to a descriptor.
 * @Arg: In: nFd = destination descriptor.
 *       In: psText = NUL-terminated text to write.
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR on failure.
 ***********************************************/
int writeString(int nFd, const char *psText);

/***********************************************
 * @Name: writeFormatted
 * @Def: Formats a message in memory with vasprintf, writes it, then
 *       frees the buffer. Never reads/frees the output pointer when
 *       formatting itself failed.
 * @Arg: In: nFd = destination descriptor.
 *       In: psFormat = printf-style format string.
 *       In: ... = format arguments.
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR if formatting or writing
 *       failed.
 ***********************************************/
int writeFormatted(int nFd, const char *psFormat, ...);

/***********************************************
 * @Name: safeRead
 * @Def: Reads from a descriptor, retrying automatically on EINTR.
 * @Arg: In: nFd = source descriptor.
 *       Out: pBuffer = destination buffer.
 *       In: nCount = maximum bytes to read.
 * @Ret: Number of bytes read (0 means EOF), or -1 on a real error with
 *       errno set by the underlying read().
 ***********************************************/
ssize_t safeRead(int nFd, void *pBuffer, size_t nCount);

/***********************************************
 * @Name: safeOpenReadOnly
 * @Def: Opens a file read-only, retrying automatically on EINTR.
 * @Arg: In: psPath = path to open.
 * @Ret: An open descriptor on success, or -1 on failure with errno set.
 ***********************************************/
int safeOpenReadOnly(const char *psPath);

#endif

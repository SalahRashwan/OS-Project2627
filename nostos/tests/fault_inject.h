#ifndef NOSTOS_FAULT_INJECT_H
#define NOSTOS_FAULT_INJECT_H

/*
 * @File: fault_inject.h
 * @Purpose: Test-only declarations for the linker-wrapper fault
 *           injector (see fault_inject.c). Never part of the delivered
 *           executables.
 * @Author: Salah Ahmed Salaheldin Adly Rashwan
 * @Date: 2026-09-26
 */

/* System Includes */
#include <errno.h>
#include <stdarg.h>
/* Only snprintf(), an in-memory formatter, is used from stdio.h. */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

/* Environment variables that select the failing call. */
#define FAULT_ALLOC_ENV "NOSTOS_FAIL_ALLOC"
#define FAULT_WRITE_ENV "NOSTOS_FAIL_STDOUT_WRITE"
/* Calibration: report the number of counted allocation calls at exit. */
#define FAULT_REPORT_ENV "NOSTOS_FAULT_REPORT"
#define FAULT_REPORT_SIZE 64

/* Real implementations provided by the linker's --wrap option. */
void *__real_malloc(size_t nSize);
void *__real_realloc(void *pBlock, size_t nSize);
int __real_vasprintf(char **ppsOut, const char *psFormat, va_list stArgs);
ssize_t __real_write(int nFd, const void *pBuffer, size_t nCount);

/* Wrappers that the linked project objects call instead. */
void *__wrap_malloc(size_t nSize);
void *__wrap_realloc(void *pBlock, size_t nSize);
int __wrap_vasprintf(char **ppsOut, const char *psFormat, va_list stArgs);
ssize_t __wrap_write(int nFd, const void *pBuffer, size_t nCount);

#endif

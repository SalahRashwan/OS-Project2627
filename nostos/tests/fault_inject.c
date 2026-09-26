/*
 * @File: fault_inject.c
 * @Purpose: Test-only linker wrappers (never linked into the delivered
 *           odysseus/ithaca/island). `make fault-bins` links the real
 *           project objects with -Wl,--wrap=malloc,--wrap=realloc,
 *           --wrap=vasprintf,--wrap=write so tests/run_fault_tests.sh can
 *           make one chosen call fail:
 *             NOSTOS_FAIL_ALLOC=N        the Nth malloc/realloc/vasprintf
 *                                        call made by linked objects
 *                                        fails with ENOMEM (1-based).
 *             NOSTOS_FAIL_STDOUT_WRITE=N the Nth and every later write()
 *                                        to STDOUT_FILENO fails with
 *                                        ENOSPC.
 *           Allocations made inside libc itself are not counted.
 *           The two counters are file-scope state of this test harness
 *           only, hence the g prefix.
 * @Author: Salah Ahmed Salaheldin Adly Rashwan
 * @Date: 2026-09-26
 */

/* Own */
#include "fault_inject.h"

static int gnAllocCalls = 0;
static int gnStdoutWrites = 0;

/***********************************************
 * @Name: readFaultIndex
 * @Def: Reads a positive fault index from an environment variable.
 * @Arg: In: psName = environment variable name.
 * @Ret: The index, or 0 when the variable is unset or not positive.
 ***********************************************/
static int readFaultIndex(const char *psName) {
    const char *psValue = getenv(psName);

    if (NULL == psValue) {
        return 0;
    }
    return atoi(psValue);
}

/***********************************************
 * @Name: shouldFailAllocation
 * @Def: Counts one allocation call and decides whether it must fail.
 * @Arg: None.
 * @Ret: 1 if this call is the configured failing call, 0 otherwise.
 ***********************************************/
static int shouldFailAllocation(void) {
    int nTarget = readFaultIndex(FAULT_ALLOC_ENV);

    gnAllocCalls++;
    if (0 < nTarget && nTarget == gnAllocCalls) {
        errno = ENOMEM;
        return 1;
    }
    return 0;
}

/***********************************************
 * @Name: reportAllocationCount
 * @Def: Runs automatically at normal process exit. When the calibration
 *       variable NOSTOS_FAULT_REPORT is set, writes the number of
 *       counted allocation calls to stderr so the sweep knows its range.
 * @Arg: None.
 * @Ret: None.
 ***********************************************/
__attribute__((destructor)) static void reportAllocationCount(void) {
    char acLine[FAULT_REPORT_SIZE];
    int nLength = 0;

    if (NULL == getenv(FAULT_REPORT_ENV)) {
        return;
    }
    nLength = snprintf(acLine, sizeof(acLine), "alloc_calls=%d\n", gnAllocCalls);
    if (0 < nLength && FAULT_REPORT_SIZE > nLength) {
        (void) __real_write(STDERR_FILENO, acLine, (size_t) nLength);
    }
}

/***********************************************
 * @Name: __wrap_malloc
 * @Def: malloc() replacement seen by the linked project objects.
 * @Arg: In: nSize = requested size.
 * @Ret: NULL when this call is chosen to fail, otherwise malloc's result.
 ***********************************************/
void *__wrap_malloc(size_t nSize) {
    if (shouldFailAllocation()) {
        return NULL;
    }
    return __real_malloc(nSize);
}

/***********************************************
 * @Name: __wrap_realloc
 * @Def: realloc() replacement; a failure leaves pBlock untouched, as
 *       the real realloc() does.
 * @Arg: In: pBlock = block to resize.
 *       In: nSize = requested size.
 * @Ret: NULL when this call is chosen to fail, otherwise realloc's
 *       result.
 ***********************************************/
void *__wrap_realloc(void *pBlock, size_t nSize) {
    if (shouldFailAllocation()) {
        return NULL;
    }
    return __real_realloc(pBlock, nSize);
}

/***********************************************
 * @Name: __wrap_vasprintf
 * @Def: vasprintf() replacement; a failure returns -1 without setting a
 *       usable output pointer, as glibc documents.
 * @Arg: Out: ppsOut = formatted string on success.
 *       In: psFormat = format string.
 *       In: stArgs = format arguments.
 * @Ret: -1 when this call is chosen to fail, otherwise vasprintf's
 *       result.
 ***********************************************/
int __wrap_vasprintf(char **ppsOut, const char *psFormat, va_list stArgs) {
    if (shouldFailAllocation()) {
        return -1;
    }
    return __real_vasprintf(ppsOut, psFormat, stArgs);
}

/***********************************************
 * @Name: __wrap_write
 * @Def: write() replacement that fails stdout writes from the configured
 *       index onwards; every other descriptor is passed through.
 * @Arg: In: nFd = destination descriptor.
 *       In: pBuffer = bytes to write.
 *       In: nCount = number of bytes.
 * @Ret: -1 with errno ENOSPC for a failing stdout write, otherwise
 *       write's result.
 ***********************************************/
ssize_t __wrap_write(int nFd, const void *pBuffer, size_t nCount) {
    int nTarget = readFaultIndex(FAULT_WRITE_ENV);

    if (STDOUT_FILENO == nFd && 0 < nTarget) {
        gnStdoutWrites++;
        if (nTarget <= gnStdoutWrites) {
            errno = ENOSPC;
            return -1;
        }
    }
    return __real_write(nFd, pBuffer, nCount);
}

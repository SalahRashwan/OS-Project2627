/*
 * @File: lifecycle.c
 * @Purpose: Race-free CTRL+C handling shared by all three processes.
 *           SIGINT stays blocked for the entire process lifetime, so
 *           no signal handler ever runs anywhere in this codebase; the
 *           signal is only ever observed by reading a signalfd inside
 *           an ordinary blocking poll().
 * @Author: Salah Ahmed Salaheldin Adly Rashwan
 * @Date: 2026-09-21
 */

/* System */
#include <errno.h>
#include <poll.h>
#include <sys/signalfd.h>

/* Own */
#include "lifecycle.h"
#include "io.h"
#include "status.h"

/***********************************************
 * @Name: blockSigint
 * @Def: See lifecycle.h.
 ***********************************************/
int blockSigint(sigset_t *pstOldMask) {
    sigset_t stSet;

    if (0 != sigemptyset(&stSet)) {
        return NOSTOS_ERROR;
    }
    if (0 != sigaddset(&stSet, SIGINT)) {
        return NOSTOS_ERROR;
    }
    if (0 != sigprocmask(SIG_BLOCK, &stSet, pstOldMask)) {
        return NOSTOS_ERROR;
    }
    return NOSTOS_OK;
}

/***********************************************
 * @Name: createSigintFd
 * @Def: See lifecycle.h.
 ***********************************************/
int createSigintFd(void) {
    sigset_t stSet;

    if (0 != sigemptyset(&stSet)) {
        return -1;
    }
    if (0 != sigaddset(&stSet, SIGINT)) {
        return -1;
    }
    return signalfd(-1, &stSet, SFD_CLOEXEC);
}

/***********************************************
 * @Name: consumeSignal
 * @Def: See lifecycle.h.
 ***********************************************/
int consumeSignal(int nSigFd) {
    struct signalfd_siginfo stInfo;
    ssize_t nRead = 0;

    nRead = safeRead(nSigFd, &stInfo, sizeof(stInfo));
    if ((ssize_t) sizeof(stInfo) != nRead) {
        return NOSTOS_ERROR;
    }
    return NOSTOS_OK;
}

/***********************************************
 * @Name: waitForSignalOnly
 * @Def: See lifecycle.h.
 ***********************************************/
int waitForSignalOnly(int nSigFd) {
    struct pollfd stPoll;
    int nReady = 0;

    stPoll.fd = nSigFd;
    stPoll.events = POLLIN;
    for (;;) {
        stPoll.revents = 0;
        nReady = poll(&stPoll, 1, -1);
        if (0 > nReady) {
            if (EINTR == errno) {
                continue;
            }
            return NOSTOS_ERROR;
        }
        if (0 < nReady && 0 != (stPoll.revents & POLLIN)) {
            return consumeSignal(nSigFd);
        }
    }
}

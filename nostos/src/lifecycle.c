/*
 * @File: lifecycle.c
 * @Purpose: Race-free CTRL+C handling shared by all three processes.
 *           SIGINT stays blocked for the entire process lifetime, so
 *           no signal handler ever runs anywhere in this codebase; the
 *           signal is only ever observed by reading a signalfd inside
 *           an ordinary blocking poll().
 * @Author: Salah Ahmed Salaheldin Adly Rashwan
 * @Date: 2026-09-26
 */

/* Own */
#include "lifecycle.h"
#include "io.h"
#include "status.h"

/***********************************************
 * @Name: blockSigint
 * @Def: Blocks SIGINT for the calling process so it is queued, never
 *       delivered as a handler interruption, and never lost between
 *       initialization steps.
 * @Arg: Out: pstOldMask = the previous signal mask (kept for reference;
 *       the process exits instead of restoring it).
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR if the mask could not be
 *       built or installed.
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
 * @Def: Creates a close-on-exec signalfd bound to a set containing only
 *       SIGINT. Must be called after blockSigint().
 * @Arg: None.
 * @Ret: An open descriptor on success (owned by the caller, closed once
 *       during cleanup), or -1 on failure.
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
 * @Def: Reads and discards exactly one struct signalfd_siginfo from a
 *       ready signalfd, so the pending SIGINT is accepted.
 * @Arg: In: nSigFd = a signalfd that poll() reported as readable.
 * @Ret: NOSTOS_OK if one complete record was read, NOSTOS_ERROR on a
 *       read failure or a short read.
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
 * @Def: Blocks in poll() on exactly one descriptor (the signalfd) with
 *       an infinite timeout, then consumes the pending signal. Used by
 *       Ithaca and Island, which have no terminal to also watch.
 * @Arg: In: nSigFd = descriptor from createSigintFd().
 * @Ret: NOSTOS_OK once SIGINT has been consumed, NOSTOS_ERROR on a
 *       poll or read failure, or if the descriptor reports an error.
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
        if (0 != (stPoll.revents & POLLIN)) {
            return consumeSignal(nSigFd);
        }
        if (0 != (stPoll.revents & (POLLERR | POLLHUP | POLLNVAL))) {
            return NOSTOS_ERROR;
        }
    }
}

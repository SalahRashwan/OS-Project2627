#ifndef NOSTOS_LIFECYCLE_H
#define NOSTOS_LIFECYCLE_H

/*
 * @File: lifecycle.h
 * @Purpose: Race-free CTRL+C handling shared by all three processes:
 *           block SIGINT before any resource is acquired, wait for it
 *           through a signalfd descriptor inside an ordinary blocking
 *           poll(), and consume it without ever running signal-handler
 *           code (there is no handler at all). See docs/design.md
 *           section 6 and the UNIX book section 12.6.
 * @Author: Salah Ahmed Salaheldin Adly Rashwan
 * @Date: 2026-09-21
 */

/* System Includes */
#include <signal.h>

/***********************************************
 * @Name: blockSigint
 * @Def: Blocks SIGINT for the calling process so it is queued, never
 *       delivered as a handler interruption, and never lost between
 *       initialization steps.
 * @Arg: Out: pstOldMask = the process's previous signal mask, saved
 *       for documentation purposes (not restored; the process exits
 *       instead of resuming normal-signal operation).
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR on failure.
 ***********************************************/
int blockSigint(sigset_t *pstOldMask);

/***********************************************
 * @Name: createSigintFd
 * @Def: Creates a signalfd bound to a set containing only SIGINT.
 *       Must be called after blockSigint(). The returned descriptor is
 *       ordinary application-owned state (close it once during
 *       cleanup), not a global.
 * @Arg: None.
 * @Ret: An open descriptor on success, or -1 on failure.
 ***********************************************/
int createSigintFd(void);

/***********************************************
 * @Name: waitForSignalOnly
 * @Def: Blocks in poll() on exactly one descriptor (the signalfd)
 *       with an infinite timeout, then consumes the pending signal.
 *       Used by Ithaca and Island, which have no terminal to also
 *       watch.
 * @Arg: In: nSigFd = descriptor from createSigintFd().
 * @Ret: NOSTOS_OK once SIGINT has been consumed, NOSTOS_ERROR on a
 *       poll/read failure.
 ***********************************************/
int waitForSignalOnly(int nSigFd);

/***********************************************
 * @Name: consumeSignal
 * @Def: Reads and discards exactly one struct signalfd_siginfo from a
 *       ready signalfd, so the same descriptor does not stay readable
 *       forever after a poll() wakeup.
 * @Arg: In: nSigFd = a signalfd that poll() reported as readable.
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR on a read failure.
 ***********************************************/
int consumeSignal(int nSigFd);

#endif

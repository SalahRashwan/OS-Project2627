#ifndef NOSTOS_LIFECYCLE_H
#define NOSTOS_LIFECYCLE_H

/*
 * @File: lifecycle.h
 * @Purpose: Race-free CTRL+C handling shared by all three processes:
 *           block SIGINT before any resource is acquired, wait for it
 *           through a signalfd descriptor inside an ordinary blocking
 *           poll(), and consume it without ever running signal-handler
 *           code (there is no handler at all). See the UNIX book
 *           section 12.6. Also sets SIGPIPE to be ignored, so a write
 *           to a pipe with no reader fails with EPIPE and goes through
 *           the normal write-error path instead of killing the process.
 * @Author: Daros Aragao Santos
 * @Date: 2026-09-29
 */

/* System Includes */
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <string.h>
#include <sys/signalfd.h>

/* Fixed stderr diagnostic used when SIGPIPE cannot be ignored. */
#define ERROR_SIGPIPE_SETUP "Error: could not set up SIGPIPE handling.\n"

/***********************************************
 * @Name: ignoreSigpipe
 * @Def: Sets the SIGPIPE action to SIG_IGN with a checked sigaction()
 *       call. Afterwards a write to a pipe with no reader returns -1
 *       with errno EPIPE, which writeAll() reports as a normal write
 *       failure. No handler function is installed. Must be the first
 *       thing main() does, before any stdout or stderr write.
 * @Arg: None.
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR if the action could not be
 *       built or installed.
 ***********************************************/
int ignoreSigpipe(void);

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

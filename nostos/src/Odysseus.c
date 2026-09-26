/*
 * @File: Odysseus.c
 * @Purpose: Entry point for the Odysseus process. Usage:
 *           ./odysseus <config.dat>
 *           The only Phase 1 process with an interactive terminal
 *           (P p.15). Recognizes the eleven commands, validates their
 *           syntax, and extracts their arguments, without implementing
 *           any command's functionality yet. Blocks (no busy waiting)
 *           on stdin and CTRL+C simultaneously via poll() over one
 *           signalfd, so a single CTRL+C at any point in a line
 *           terminates cleanly. Every output, read, and allocation
 *           failure is reported on stderr and ends the process with a
 *           failure status after releasing its resources.
 * @Author: Salah Ahmed Salaheldin Adly Rashwan
 * @Date: 2026-09-26
 */

#include "Odysseus.h"

/***********************************************
 * @Name: reportTerminalError
 * @Def: Writes one fixed diagnostic literal to stderr. The literal needs
 *       no heap allocation, and the result of this single attempt is
 *       deliberately not retried, so a broken stderr can never block
 *       cleanup or cause an endless retry loop.
 * @Arg: In: psMessage = NUL-terminated diagnostic literal.
 * @Ret: NOSTOS_ERROR, always, so callers can return it directly.
 ***********************************************/
static int reportTerminalError(const char *psMessage) {
    /* One best-effort attempt: if stderr is broken too, nothing further
     * can be reported, and the caller still performs full cleanup. */
    (void) writeString(STDERR_FILENO, psMessage);
    return NOSTOS_ERROR;
}

/***********************************************
 * @Name: initializeOdysseus
 * @Def: Blocks SIGINT, creates its signalfd, then loads the Odysseus
 *       configuration. Releases the signalfd itself if loading fails.
 * @Arg: In: psConfigPath = path to odysseus.dat.
 *       Out: pnSigFd = open signalfd on success.
 *       Out: pstConfig = loaded configuration on success.
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR otherwise.
 ***********************************************/
static int initializeOdysseus(const char *psConfigPath, int *pnSigFd, tOdysseusConfig *pstConfig) {
    sigset_t stOldMask;

    if (NOSTOS_OK != blockSigint(&stOldMask)) {
        return NOSTOS_ERROR;
    }
    *pnSigFd = createSigintFd();
    if (-1 == *pnSigFd) {
        return NOSTOS_ERROR;
    }
    if (NOSTOS_OK != loadOdysseusConfig(psConfigPath, pstConfig)) {
        close(*pnSigFd);
        *pnSigFd = -1;
        return NOSTOS_ERROR;
    }
    return NOSTOS_OK;
}

/***********************************************
 * @Name: printPrompt
 * @Def: Prints the "$ " terminal prompt and reports a failed write.
 * @Arg: None.
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR if the prompt could not be
 *       written (a diagnostic has then been attempted on stderr).
 ***********************************************/
static int printPrompt(void) {
    if (NOSTOS_OK != writeString(STDOUT_FILENO, TERMINAL_PROMPT)) {
        return reportTerminalError(ERROR_TERMINAL_WRITE);
    }
    return NOSTOS_OK;
}

/***********************************************
 * @Name: printParseResult
 * @Def: Writes exactly the lines required for one parse result:
 *       Command OK, Unknown command, one or two usage lines, or nothing
 *       for a blank line (assumption A08).
 * @Arg: In: eStatus = parser result other than PARSE_ERROR.
 *       In: pstUsage = usage lines, read only for PARSE_USAGE.
 * @Ret: NOSTOS_OK if every required byte was written, NOSTOS_ERROR
 *       otherwise.
 ***********************************************/
static int printParseResult(eParseStatus eStatus, const tUsageMessage *pstUsage) {
    int nIndex = 0;
    int nStatus = NOSTOS_OK;

    switch (eStatus) {
        case PARSE_OK:
            nStatus = writeString(STDOUT_FILENO, COMMAND_OK_MESSAGE);
            break;
        case PARSE_UNKNOWN:
            nStatus = writeString(STDOUT_FILENO, UNKNOWN_COMMAND_MESSAGE);
            break;
        case PARSE_USAGE:
            for (nIndex = 0; nIndex < pstUsage->nLineCount && NOSTOS_OK == nStatus; nIndex++) {
                nStatus = writeString(STDOUT_FILENO, pstUsage->apsLines[nIndex]);
            }
            break;
        default:
            nStatus = NOSTOS_OK;
            break;
    }
    return nStatus;
}

/***********************************************
 * @Name: executeAndPrint
 * @Def: Parses one complete command line and prints its Phase 1 result.
 *       Never touches voyage/route/market state. A parser allocation
 *       failure is reported as a resource error, never as an unknown
 *       command.
 * @Arg: In: psLine = one line, without its trailing newline.
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR on a parser allocation
 *       failure or an output write failure (diagnostic attempted).
 ***********************************************/
static int executeAndPrint(const char *psLine) {
    tParsedCommand stCommand;
    tUsageMessage stUsage;
    eParseStatus eStatus = PARSE_ERROR;
    int nStatus = NOSTOS_OK;

    eStatus = parseCommand(psLine, &stCommand, &stUsage);
    if (PARSE_ERROR == eStatus) {
        nStatus = reportTerminalError(ERROR_PARSE_MEMORY);
    } else if (NOSTOS_OK != printParseResult(eStatus, &stUsage)) {
        nStatus = reportTerminalError(ERROR_TERMINAL_WRITE);
    }
    destroyParsedCommand(&stCommand);
    return nStatus;
}

/***********************************************
 * @Name: processAvailableLines
 * @Def: Executes every complete line currently buffered, printing a
 *       fresh prompt after each one, so a pasted multi-command block
 *       or a redirected file yields exactly one result per command.
 * @Arg: In/Out: pstBuffer = terminal line accumulator.
 * @Ret: NOSTOS_OK once no complete line remains, NOSTOS_ERROR on an
 *       allocation or output failure (diagnostic attempted).
 ***********************************************/
static int processAvailableLines(tLineBuffer *pstBuffer) {
    char *psLine = NULL;
    int nExtracted = LINE_NONE;
    int nStatus = NOSTOS_OK;

    for (;;) {
        nExtracted = lineBufferExtractLine(pstBuffer, &psLine);
        if (NOSTOS_ERROR == nExtracted) {
            return reportTerminalError(ERROR_INPUT_MEMORY);
        }
        if (LINE_NONE == nExtracted) {
            return NOSTOS_OK;
        }
        nStatus = executeAndPrint(psLine);
        free(psLine);
        if (NOSTOS_OK != nStatus) {
            return NOSTOS_ERROR;
        }
        if (NOSTOS_OK != printPrompt()) {
            return NOSTOS_ERROR;
        }
    }
}

/***********************************************
 * @Name: handleStdinReadable
 * @Def: Reads one bounded chunk from stdin and either appends it to
 *       the accumulator (then executes every line it now completes),
 *       or records a clean EOF for the caller to finish once.
 * @Arg: In/Out: pstBuffer = terminal line accumulator.
 *       Out: pnEof = set to 1 when stdin reached end of file.
 * @Ret: NOSTOS_OK on success or clean EOF, NOSTOS_ERROR on a read,
 *       allocation, or output failure (diagnostic attempted).
 ***********************************************/
static int handleStdinReadable(tLineBuffer *pstBuffer, int *pnEof) {
    char acChunk[TERMINAL_READ_CHUNK_SIZE];
    ssize_t nRead = 0;

    nRead = safeRead(STDIN_FILENO, acChunk, sizeof(acChunk));
    if (0 > nRead) {
        return reportTerminalError(ERROR_TERMINAL_READ);
    }
    if (0 == nRead) {
        *pnEof = 1;
        return NOSTOS_OK;
    }
    if (NOSTOS_OK != lineBufferAppend(pstBuffer, acChunk, (size_t) nRead)) {
        return reportTerminalError(ERROR_INPUT_MEMORY);
    }
    return processAvailableLines(pstBuffer);
}

/***********************************************
 * @Name: handleEofRemainder
 * @Def: Executes the final unterminated command once, if any bytes
 *       remain unconsumed when stdin reaches EOF.
 * @Arg: In/Out: pstBuffer = terminal line accumulator; drained.
 * @Ret: NOSTOS_OK if there was no remainder or it was executed and
 *       printed; NOSTOS_ERROR if the remainder could not be extracted
 *       (allocation failure) or its execution failed (diagnostic
 *       attempted in both cases).
 ***********************************************/
static int handleEofRemainder(tLineBuffer *pstBuffer) {
    char *psLine = NULL;
    int nTaken = LINE_NONE;
    int nStatus = NOSTOS_OK;

    nTaken = lineBufferTakeRemainder(pstBuffer, &psLine);
    if (NOSTOS_ERROR == nTaken) {
        return reportTerminalError(ERROR_INPUT_MEMORY);
    }
    if (LINE_FOUND == nTaken) {
        nStatus = executeAndPrint(psLine);
        free(psLine);
    }
    return nStatus;
}

/***********************************************
 * @Name: waitTerminalEvent
 * @Def: Blocks in poll() on stdin and the signalfd with no timeout,
 *       retrying only when poll() itself is interrupted.
 * @Arg: In/Out: astPoll = the two poll entries (stdin, signalfd);
 *       revents are refreshed.
 * @Ret: NOSTOS_OK when at least one descriptor is ready, NOSTOS_ERROR
 *       if poll() failed (diagnostic attempted).
 ***********************************************/
static int waitTerminalEvent(struct pollfd astPoll[TERMINAL_POLL_COUNT]) {
    int nReady = 0;

    for (;;) {
        astPoll[0].revents = 0;
        astPoll[1].revents = 0;
        nReady = poll(astPoll, TERMINAL_POLL_COUNT, -1);
        if (0 < nReady) {
            return NOSTOS_OK;
        }
        if (0 > nReady && EINTR != errno) {
            return reportTerminalError(ERROR_TERMINAL_WAIT);
        }
    }
}

/***********************************************
 * @Name: runOdysseusTerminal
 * @Def: The main terminal loop: watches stdin and the signalfd
 *       together so a partial command line is discarded cleanly the
 *       instant CTRL+C arrives, and EOF ends the terminal once.
 * @Arg: In: nSigFd = signalfd created by initializeOdysseus().
 * @Ret: NOSTOS_OK on CTRL+C or clean EOF, NOSTOS_ERROR on any I/O,
 *       signal, or allocation failure (diagnostic attempted).
 ***********************************************/
static int runOdysseusTerminal(int nSigFd) {
    tLineBuffer stBuffer;
    struct pollfd astPoll[TERMINAL_POLL_COUNT];
    int nEof = 0;
    int nStatus = NOSTOS_OK;

    lineBufferInit(&stBuffer);
    astPoll[0].fd = STDIN_FILENO;
    astPoll[0].events = POLLIN;
    astPoll[1].fd = nSigFd;
    astPoll[1].events = POLLIN;
    nStatus = printPrompt();
    while (NOSTOS_OK == nStatus && 0 == nEof) {
        nStatus = waitTerminalEvent(astPoll);
        if (NOSTOS_OK != nStatus) {
            break;
        }
        if (0 != (astPoll[1].revents & POLLIN)) {
            if (NOSTOS_OK != consumeSignal(nSigFd)) {
                nStatus = reportTerminalError(ERROR_SIGNAL_READ);
            }
            break;
        }
        if (0 != (astPoll[0].revents & (POLLIN | POLLHUP | POLLERR | POLLNVAL))) {
            nStatus = handleStdinReadable(&stBuffer, &nEof);
        }
    }
    if (NOSTOS_OK == nStatus && 0 != nEof) {
        nStatus = handleEofRemainder(&stBuffer);
    }
    lineBufferDestroy(&stBuffer);
    return nStatus;
}

/***********************************************
 * @Name: main
 * @Def: Entry point. Validates the CLI argument count, initializes,
 *       prints the readiness message, runs the terminal, releases every
 *       owned resource, and returns the documented exit status
 *       (assumption A21).
 * @Arg: In: argc = argument count.
 *       In: argv = argument vector; argv[1] = odysseus.dat path.
 * @Ret: NOSTOS_EXIT_OK on CTRL+C/EOF, NOSTOS_EXIT_ARGS on a wrong
 *       argument count, NOSTOS_EXIT_IO on any initialization, I/O, or
 *       allocation failure.
 ***********************************************/
int main(int argc, char *argv[]) {
    int nSigFd = -1;
    tOdysseusConfig stConfig;
    int nStatus = NOSTOS_OK;

    if (2 != argc) {
        (void) writeString(STDERR_FILENO, "Usage: odysseus <config.dat>\n");
        return NOSTOS_EXIT_ARGS;
    }
    if (NOSTOS_OK != initializeOdysseus(argv[1], &nSigFd, &stConfig)) {
        (void) writeString(STDERR_FILENO, "Error: Odysseus failed to initialize.\n");
        return NOSTOS_EXIT_IO;
    }
    if (NOSTOS_OK != writeFormatted(STDOUT_FILENO, "Odysseus %s is ready to sail.\n", stConfig.psName)) {
        nStatus = reportTerminalError(ERROR_TERMINAL_WRITE);
    } else {
        nStatus = runOdysseusTerminal(nSigFd);
    }
    destroyOdysseusConfig(&stConfig);
    close(nSigFd);
    if (NOSTOS_OK != nStatus) {
        return NOSTOS_EXIT_IO;
    }
    return NOSTOS_EXIT_OK;
}

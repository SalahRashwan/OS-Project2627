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
 *           terminates cleanly.
 * @Author: Salah Ahmed Salaheldin Adly Rashwan
 * @Date: 2026-09-21
 */

#include "Odysseus.h"

/***********************************************
 * @Name: initializeOdysseus
 * @Def: Blocks SIGINT, creates its signalfd, then loads the Odysseus
 *       configuration.
 * @Arg: In: psConfigPath. Out: pnSigFd, pstConfig.
 * @Ret: NOSTOS_OK / NOSTOS_ERROR.
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
        return NOSTOS_ERROR;
    }
    return NOSTOS_OK;
}

/***********************************************
 * @Name: announceOdysseusReady
 * @Def: Prints the required readiness message with the actual loaded
 *       ship name.
 * @Arg: In: pstConfig.
 * @Ret: NOSTOS_OK / NOSTOS_ERROR.
 ***********************************************/
static int announceOdysseusReady(const tOdysseusConfig *pstConfig) {
    return writeFormatted(STDOUT_FILENO, "Odysseus %s is ready to sail.\n", pstConfig->psName);
}

/***********************************************
 * @Name: printPrompt
 * @Def: Prints the "$ " terminal prompt.
 * @Arg: None.
 * @Ret: NOSTOS_OK / NOSTOS_ERROR.
 ***********************************************/
static int printPrompt(void) {
    return writeString(STDOUT_FILENO, "$ ");
}

/***********************************************
 * @Name: executeAndPrint
 * @Def: Parses one complete command line and prints exactly the
 *       result Phase 1 requires: Command OK, Unknown command, one or
 *       two usage lines, or nothing at all for a blank line
 *       (assumption A08). Never touches voyage/route/market state.
 * @Arg: In: psLine = one line, without its trailing newline.
 * @Ret: None.
 ***********************************************/
static void executeAndPrint(const char *psLine) {
    tParsedCommand stCommand;
    tUsageMessage stUsage;
    eParseStatus eStatus;
    int nIndex = 0;

    eStatus = parseCommand(psLine, &stCommand, &stUsage);
    if (PARSE_OK == eStatus) {
        writeString(STDOUT_FILENO, COMMAND_OK_MESSAGE);
    } else if (PARSE_UNKNOWN == eStatus) {
        writeString(STDOUT_FILENO, UNKNOWN_COMMAND_MESSAGE);
    } else if (PARSE_USAGE == eStatus) {
        for (nIndex = 0; nIndex < stUsage.nLineCount; nIndex++) {
            writeString(STDOUT_FILENO, stUsage.apsLines[nIndex]);
        }
    }
    destroyParsedCommand(&stCommand);
}

/***********************************************
 * @Name: processAvailableLines
 * @Def: Executes every complete line currently buffered, printing a
 *       fresh prompt after each one, so a pasted multi-command block
 *       or a redirected file yields exactly one result per command.
 * @Arg: In/Out: pstBuffer = terminal line accumulator.
 * @Ret: NOSTOS_OK / NOSTOS_ERROR.
 ***********************************************/
static int processAvailableLines(tLineBuffer *pstBuffer) {
    char *psLine = NULL;
    int nExtracted = 0;

    for (;;) {
        nExtracted = lineBufferExtractLine(pstBuffer, &psLine);
        if (NOSTOS_ERROR == nExtracted) {
            return NOSTOS_ERROR;
        }
        if (LINE_NONE == nExtracted) {
            return NOSTOS_OK;
        }
        executeAndPrint(psLine);
        free(psLine);
        printPrompt();
    }
}

/***********************************************
 * @Name: handleStdinReadable
 * @Def: Reads one bounded chunk from stdin and either appends it to
 *       the accumulator (then executes every line it now completes),
 *       or records a clean EOF for the caller to finish once.
 * @Arg: In/Out: pstBuffer. Out: pbEof = set to 1 on EOF.
 * @Ret: NOSTOS_OK / NOSTOS_ERROR.
 ***********************************************/
static int handleStdinReadable(tLineBuffer *pstBuffer, int *pbEof) {
    char acChunk[TERMINAL_READ_CHUNK_SIZE];
    ssize_t nRead = 0;

    nRead = safeRead(STDIN_FILENO, acChunk, sizeof(acChunk));
    if (0 > nRead) {
        return NOSTOS_ERROR;
    }
    if (0 == nRead) {
        *pbEof = 1;
        return NOSTOS_OK;
    }
    if (NOSTOS_OK != lineBufferAppend(pstBuffer, acChunk, (size_t) nRead)) {
        return NOSTOS_ERROR;
    }
    return processAvailableLines(pstBuffer);
}

/***********************************************
 * @Name: handleEofRemainder
 * @Def: Executes the final unterminated command once, if any bytes
 *       remain unconsumed when stdin reaches EOF.
 * @Arg: In/Out: pstBuffer.
 * @Ret: None.
 ***********************************************/
static void handleEofRemainder(tLineBuffer *pstBuffer) {
    char *psLine = NULL;

    if (LINE_FOUND == lineBufferTakeRemainder(pstBuffer, &psLine)) {
        executeAndPrint(psLine);
        free(psLine);
    }
}

/***********************************************
 * @Name: runOdysseusTerminal
 * @Def: The main poll() loop: watches stdin and the signalfd
 *       together so a partial command line is discarded cleanly the
 *       instant CTRL+C arrives, and EOF ends the terminal without an
 *       infinite loop.
 * @Arg: In: nSigFd.
 * @Ret: NOSTOS_OK / NOSTOS_ERROR.
 ***********************************************/
static int runOdysseusTerminal(int nSigFd) {
    tLineBuffer stBuffer;
    struct pollfd astPoll[2];
    int nReady = 0;
    int bEof = 0;
    int nStatus = NOSTOS_OK;

    lineBufferInit(&stBuffer);
    printPrompt();
    astPoll[0].fd = STDIN_FILENO;
    astPoll[0].events = POLLIN;
    astPoll[1].fd = nSigFd;
    astPoll[1].events = POLLIN;
    for (;;) {
        astPoll[0].revents = 0;
        astPoll[1].revents = 0;
        nReady = poll(astPoll, 2, -1);
        if (0 > nReady) {
            if (EINTR == errno) {
                continue;
            }
            nStatus = NOSTOS_ERROR;
            break;
        }
        if (0 != (astPoll[1].revents & POLLIN)) {
            consumeSignal(nSigFd);
            break;
        }
        if (0 != (astPoll[0].revents & (POLLIN | POLLHUP | POLLERR | POLLNVAL))) {
            if (NOSTOS_OK != handleStdinReadable(&stBuffer, &bEof)) {
                nStatus = NOSTOS_ERROR;
                break;
            }
            if (bEof) {
                handleEofRemainder(&stBuffer);
                break;
            }
        }
    }
    lineBufferDestroy(&stBuffer);
    return nStatus;
}

/***********************************************
 * @Name: main
 * @Def: Entry point. Validates the CLI argument count, initializes,
 *       runs the terminal, and returns the documented exit status
 *       (assumption A21).
 * @Arg: In: argc = argument count. In: argv[1] = config.dat path.
 * @Ret: NOSTOS_EXIT_OK / NOSTOS_EXIT_ARGS / NOSTOS_EXIT_IO.
 ***********************************************/
int main(int argc, char *argv[]) {
    int nSigFd = -1;
    tOdysseusConfig stConfig;
    int nStatus = NOSTOS_OK;

    if (2 != argc) {
        writeString(STDERR_FILENO, "Usage: odysseus <config.dat>\n");
        return NOSTOS_EXIT_ARGS;
    }
    if (NOSTOS_OK != initializeOdysseus(argv[1], &nSigFd, &stConfig)) {
        writeString(STDERR_FILENO, "Error: Odysseus failed to initialize.\n");
        return NOSTOS_EXIT_IO;
    }
    if (NOSTOS_OK != announceOdysseusReady(&stConfig)) {
        nStatus = NOSTOS_ERROR;
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

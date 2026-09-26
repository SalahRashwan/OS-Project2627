/*
 * @File: commands.c
 * @Purpose: Syntax-only Odysseus command parser. Pure classification:
 *           no descriptor is opened, no voyage/route/market state is
 *           consulted, and nothing is printed here (see Odysseus.c).
 * @Author: Salah Ahmed Salaheldin Adly Rashwan
 * @Date: 2026-09-21
 */

/* System */
#include <limits.h>
#include <stdlib.h>
#include <string.h>

/* Own */
#include "commands.h"
#include "text.h"
#include "status.h"

/***********************************************
 * @Name: classifyNoArgCommand
 * @Def: Validates a single-token command (MAP/STATUS/DELIVER/CLAIM).
 * @Arg: In: nTokenCount, psUsageText, eKind.
 *       Out: pstCommand, pstUsage.
 * @Ret: PARSE_OK / PARSE_USAGE.
 ***********************************************/
static eParseStatus classifyNoArgCommand(int nTokenCount, const char *psUsageText, eCommandKind eKind,
                                          tParsedCommand *pstCommand, tUsageMessage *pstUsage) {
    if (1 != nTokenCount) {
        pstUsage->apsLines[0] = psUsageText;
        pstUsage->nLineCount = 1;
        return PARSE_USAGE;
    }
    pstCommand->eKind = eKind;
    return PARSE_OK;
}

/***********************************************
 * @Name: classifyConnect
 * @Def: Validates "CONNECT ITHACA".
 * @Arg: In: appsTokens, nTokenCount. Out: pstCommand, pstUsage.
 * @Ret: PARSE_OK / PARSE_USAGE.
 ***********************************************/
static eParseStatus classifyConnect(char **appsTokens, int nTokenCount, tParsedCommand *pstCommand,
                                     tUsageMessage *pstUsage) {
    if (2 != nTokenCount || 0 != strcasecmp("ITHACA", appsTokens[1])) {
        pstUsage->apsLines[0] = USAGE_CONNECT;
        pstUsage->nLineCount = 1;
        return PARSE_USAGE;
    }
    pstCommand->eKind = CMD_CONNECT_ITHACA;
    return PARSE_OK;
}

/***********************************************
 * @Name: classifyList
 * @Def: Validates "LIST VOYAGES" / "LIST MARKET"; a missing or
 *       unrecognized subcommand shows both usages (assumption A09).
 * @Arg: In: appsTokens, nTokenCount. Out: pstCommand, pstUsage.
 * @Ret: PARSE_OK / PARSE_USAGE.
 ***********************************************/
static eParseStatus classifyList(char **appsTokens, int nTokenCount, tParsedCommand *pstCommand,
                                  tUsageMessage *pstUsage) {
    int bIsVoyages = 0;
    int bIsMarket = 0;

    if (2 <= nTokenCount) {
        bIsVoyages = (0 == strcasecmp("VOYAGES", appsTokens[1]));
        bIsMarket = (0 == strcasecmp("MARKET", appsTokens[1]));
    }
    if (2 == nTokenCount && bIsVoyages) {
        pstCommand->eKind = CMD_LIST_VOYAGES;
        return PARSE_OK;
    }
    if (2 == nTokenCount && bIsMarket) {
        pstCommand->eKind = CMD_LIST_MARKET;
        return PARSE_OK;
    }
    if (bIsVoyages) {
        pstUsage->apsLines[0] = USAGE_LIST_VOYAGES;
        pstUsage->nLineCount = 1;
        return PARSE_USAGE;
    }
    if (bIsMarket) {
        pstUsage->apsLines[0] = USAGE_LIST_MARKET;
        pstUsage->nLineCount = 1;
        return PARSE_USAGE;
    }
    pstUsage->apsLines[0] = USAGE_LIST_VOYAGES;
    pstUsage->apsLines[1] = USAGE_LIST_MARKET;
    pstUsage->nLineCount = 2;
    return PARSE_USAGE;
}

/***********************************************
 * @Name: classifyAccept
 * @Def: Validates "ACCEPT <voyage_id>": one digit-only token, bounded
 *       to [1, INT_MAX] (assumptions A05, A17). No voyage-list lookup.
 * @Arg: In: appsTokens, nTokenCount. Out: pstCommand, pstUsage.
 * @Ret: PARSE_OK / PARSE_USAGE.
 ***********************************************/
static eParseStatus classifyAccept(char **appsTokens, int nTokenCount, tParsedCommand *pstCommand,
                                    tUsageMessage *pstUsage) {
    long lValue = 0;
    int nValid = 0;

    nValid = (2 == nTokenCount) && (NOSTOS_OK == parseDigitsToLong(appsTokens[1], &lValue));
    if (nValid && (lValue < 1 || lValue > INT_MAX)) {
        nValid = 0;
    }
    if (0 == nValid) {
        pstUsage->apsLines[0] = USAGE_ACCEPT;
        pstUsage->nLineCount = 1;
        return PARSE_USAGE;
    }
    pstCommand->eKind = CMD_ACCEPT;
    pstCommand->lNumber = lValue;
    pstCommand->bHasNumber = 1;
    return PARSE_OK;
}

/***********************************************
 * @Name: classifySail
 * @Def: Validates "SAIL <island>": one nonempty token, no
 *       reachability/state check (Phase 1 is syntax-only).
 * @Arg: In: appsTokens, nTokenCount. Out: pstCommand, pstUsage.
 * @Ret: PARSE_OK / PARSE_USAGE.
 ***********************************************/
static eParseStatus classifySail(char **appsTokens, int nTokenCount, tParsedCommand *pstCommand,
                                  tUsageMessage *pstUsage) {
    if (2 != nTokenCount) {
        pstUsage->apsLines[0] = USAGE_SAIL;
        pstUsage->nLineCount = 1;
        return PARSE_USAGE;
    }
    pstCommand->eKind = CMD_SAIL;
    pstCommand->psArg1 = duplicateString(appsTokens[1]);
    if (NULL == pstCommand->psArg1) {
        /* Allocation failure: degrade safely rather than report a
         * success that never actually stored the argument. */
        return PARSE_UNKNOWN;
    }
    return PARSE_OK;
}

/***********************************************
 * @Name: classifyBuySell
 * @Def: Validates "BUY/SELL <product> <amount>": nonempty product,
 *       positive digit-only amount. For BUY specifically, a literal
 *       "MAP" product must carry amount 1 (assumption A06); SELL has
 *       no such restriction (assumption A07).
 * @Arg: In: appsTokens, nTokenCount, eKind, psUsageText.
 *       Out: pstCommand, pstUsage.
 * @Ret: PARSE_OK / PARSE_USAGE.
 ***********************************************/
static eParseStatus classifyBuySell(char **appsTokens, int nTokenCount, eCommandKind eKind,
                                     const char *psUsageText, tParsedCommand *pstCommand,
                                     tUsageMessage *pstUsage) {
    long lAmount = 0;
    int nValid = 0;

    nValid = (3 == nTokenCount) && (NOSTOS_OK == parseDigitsToLong(appsTokens[2], &lAmount));
    if (nValid && (lAmount < 1 || lAmount > INT_MAX)) {
        nValid = 0;
    }
    if (nValid && CMD_BUY == eKind && 0 == strcasecmp("MAP", appsTokens[1]) && 1 != lAmount) {
        nValid = 0;
    }
    if (0 == nValid) {
        pstUsage->apsLines[0] = psUsageText;
        pstUsage->nLineCount = 1;
        return PARSE_USAGE;
    }
    pstCommand->eKind = eKind;
    pstCommand->psArg1 = duplicateString(appsTokens[1]);
    pstCommand->lNumber = lAmount;
    pstCommand->bHasNumber = 1;
    if (NULL == pstCommand->psArg1) {
        return PARSE_UNKNOWN;
    }
    return PARSE_OK;
}

/***********************************************
 * @Name: dispatchVerb
 * @Def: Looks up the exact (case-insensitive) verb and calls its
 *       classifier. An unmatched verb is PARSE_UNKNOWN, never a
 *       prefix match (e.g. "MAPS" does not match "MAP").
 * @Arg: In: appsTokens, nTokenCount. Out: pstCommand, pstUsage.
 * @Ret: The classifier's result, or PARSE_UNKNOWN.
 ***********************************************/
static eParseStatus dispatchVerb(char **appsTokens, int nTokenCount, tParsedCommand *pstCommand,
                                  tUsageMessage *pstUsage) {
    const char *psVerb = appsTokens[0];

    if (0 == strcasecmp("CONNECT", psVerb)) {
        return classifyConnect(appsTokens, nTokenCount, pstCommand, pstUsage);
    }
    if (0 == strcasecmp("LIST", psVerb)) {
        return classifyList(appsTokens, nTokenCount, pstCommand, pstUsage);
    }
    if (0 == strcasecmp("ACCEPT", psVerb)) {
        return classifyAccept(appsTokens, nTokenCount, pstCommand, pstUsage);
    }
    if (0 == strcasecmp("SAIL", psVerb)) {
        return classifySail(appsTokens, nTokenCount, pstCommand, pstUsage);
    }
    if (0 == strcasecmp("MAP", psVerb)) {
        return classifyNoArgCommand(nTokenCount, USAGE_MAP, CMD_MAP, pstCommand, pstUsage);
    }
    if (0 == strcasecmp("BUY", psVerb)) {
        return classifyBuySell(appsTokens, nTokenCount, CMD_BUY, USAGE_BUY, pstCommand, pstUsage);
    }
    if (0 == strcasecmp("SELL", psVerb)) {
        return classifyBuySell(appsTokens, nTokenCount, CMD_SELL, USAGE_SELL, pstCommand, pstUsage);
    }
    if (0 == strcasecmp("STATUS", psVerb)) {
        return classifyNoArgCommand(nTokenCount, USAGE_STATUS, CMD_STATUS, pstCommand, pstUsage);
    }
    if (0 == strcasecmp("DELIVER", psVerb)) {
        return classifyNoArgCommand(nTokenCount, USAGE_DELIVER, CMD_DELIVER, pstCommand, pstUsage);
    }
    if (0 == strcasecmp("CLAIM", psVerb)) {
        return classifyNoArgCommand(nTokenCount, USAGE_CLAIM, CMD_CLAIM, pstCommand, pstUsage);
    }
    return PARSE_UNKNOWN;
}

/***********************************************
 * @Name: parseCommand
 * @Def: See commands.h.
 ***********************************************/
eParseStatus parseCommand(const char *psLine, tParsedCommand *pstCommand, tUsageMessage *pstUsage) {
    char *psMutable = NULL;
    char **appsTokens = NULL;
    int nTokenCount = 0;
    eParseStatus eResult = PARSE_UNKNOWN;

    pstCommand->eKind = CMD_UNKNOWN;
    pstCommand->psArg1 = NULL;
    pstCommand->lNumber = 0;
    pstCommand->bHasNumber = 0;
    pstUsage->nLineCount = 0;

    /* strtok_r mutates in place; parse a private copy so the caller's
     * line (owned by the terminal's line buffer) is never modified. */
    psMutable = duplicateString(psLine);
    if (NULL == psMutable) {
        return PARSE_UNKNOWN;
    }
    if (NOSTOS_OK != tokenizeLine(psMutable, &appsTokens, &nTokenCount)) {
        free(psMutable);
        return PARSE_UNKNOWN;
    }
    if (0 == nTokenCount) {
        free(appsTokens);
        free(psMutable);
        return PARSE_EMPTY;
    }
    eResult = dispatchVerb(appsTokens, nTokenCount, pstCommand, pstUsage);
    free(appsTokens);
    free(psMutable);
    return eResult;
}

/***********************************************
 * @Name: destroyParsedCommand
 * @Def: See commands.h.
 ***********************************************/
void destroyParsedCommand(tParsedCommand *pstCommand) {
    free(pstCommand->psArg1);
    pstCommand->psArg1 = NULL;
}

/*
 * @File: commands.c
 * @Purpose: Syntax-only Odysseus command parser. Pure classification:
 *           no descriptor is opened, no voyage/route/market state is
 *           consulted, and nothing is printed here (see Odysseus.c).
 * @Author: Salah Ahmed Salaheldin Adly Rashwan
 * @Date: 2026-09-26
 */

/* Own */
#include "commands.h"
#include "text.h"
#include "status.h"

/***********************************************
 * @Name: setSingleUsage
 * @Def: Records exactly one usage line to print.
 * @Arg: Out: pstUsage = usage message to fill.
 *       In: psUsageText = borrowed usage literal.
 * @Ret: PARSE_USAGE, always, so callers can return it directly.
 ***********************************************/
static eParseStatus setSingleUsage(tUsageMessage *pstUsage, const char *psUsageText) {
    pstUsage->apsLines[0] = psUsageText;
    pstUsage->nLineCount = 1;
    return PARSE_USAGE;
}

/***********************************************
 * @Name: classifyNoArgCommand
 * @Def: Validates a single-token command (MAP/STATUS/DELIVER/CLAIM).
 * @Arg: In: nTokenCount = number of tokens on the line.
 *       In: psUsageText = usage literal for this verb.
 *       In: eKind = command kind to record on success.
 *       Out: pstCommand = receives eKind on success.
 *       Out: pstUsage = receives the usage line on failure.
 * @Ret: PARSE_OK or PARSE_USAGE.
 ***********************************************/
static eParseStatus classifyNoArgCommand(int nTokenCount, const char *psUsageText, eCommandKind eKind,
                                          tParsedCommand *pstCommand, tUsageMessage *pstUsage) {
    if (1 != nTokenCount) {
        return setSingleUsage(pstUsage, psUsageText);
    }
    pstCommand->eKind = eKind;
    return PARSE_OK;
}

/***********************************************
 * @Name: classifyConnect
 * @Def: Validates "CONNECT ITHACA" (second word case-insensitive).
 * @Arg: In: appsTokens = borrowed line tokens.
 *       In: nTokenCount = number of tokens.
 *       Out: pstCommand = receives the kind on success.
 *       Out: pstUsage = receives the usage line on failure.
 * @Ret: PARSE_OK or PARSE_USAGE.
 ***********************************************/
static eParseStatus classifyConnect(char **appsTokens, int nTokenCount, tParsedCommand *pstCommand,
                                     tUsageMessage *pstUsage) {
    if (2 != nTokenCount || 0 != strcasecmp("ITHACA", appsTokens[1])) {
        return setSingleUsage(pstUsage, USAGE_CONNECT);
    }
    pstCommand->eKind = CMD_CONNECT_ITHACA;
    return PARSE_OK;
}

/***********************************************
 * @Name: classifyList
 * @Def: Validates "LIST VOYAGES" / "LIST MARKET"; a missing or
 *       unrecognized subcommand shows both usages.
 * @Arg: In: appsTokens = borrowed line tokens.
 *       In: nTokenCount = number of tokens.
 *       Out: pstCommand = receives the kind on success.
 *       Out: pstUsage = receives one or two usage lines on failure.
 * @Ret: PARSE_OK or PARSE_USAGE.
 ***********************************************/
static eParseStatus classifyList(char **appsTokens, int nTokenCount, tParsedCommand *pstCommand,
                                  tUsageMessage *pstUsage) {
    int nIsVoyages = 0;
    int nIsMarket = 0;

    if (2 <= nTokenCount) {
        nIsVoyages = (0 == strcasecmp("VOYAGES", appsTokens[1]));
        nIsMarket = (0 == strcasecmp("MARKET", appsTokens[1]));
    }
    if (2 == nTokenCount && nIsVoyages) {
        pstCommand->eKind = CMD_LIST_VOYAGES;
        return PARSE_OK;
    }
    if (2 == nTokenCount && nIsMarket) {
        pstCommand->eKind = CMD_LIST_MARKET;
        return PARSE_OK;
    }
    if (nIsVoyages) {
        return setSingleUsage(pstUsage, USAGE_LIST_VOYAGES);
    }
    if (nIsMarket) {
        return setSingleUsage(pstUsage, USAGE_LIST_MARKET);
    }
    pstUsage->apsLines[0] = USAGE_LIST_VOYAGES;
    pstUsage->apsLines[1] = USAGE_LIST_MARKET;
    pstUsage->nLineCount = 2;
    return PARSE_USAGE;
}

/***********************************************
 * @Name: classifyAccept
 * @Def: Validates "ACCEPT <voyage_id>": one digit-only token, bounded
 *       to [1, INT_MAX]. No voyage-list lookup.
 * @Arg: In: appsTokens = borrowed line tokens.
 *       In: nTokenCount = number of tokens.
 *       Out: pstCommand = receives kind and identifier on success.
 *       Out: pstUsage = receives the usage line on failure.
 * @Ret: PARSE_OK or PARSE_USAGE.
 ***********************************************/
static eParseStatus classifyAccept(char **appsTokens, int nTokenCount, tParsedCommand *pstCommand,
                                    tUsageMessage *pstUsage) {
    long lValue = 0;
    int nValid = 0;

    nValid = (2 == nTokenCount) && (NOSTOS_OK == parseDigitsToLong(appsTokens[1], &lValue));
    if (nValid && (1 > lValue || INT_MAX < lValue)) {
        nValid = 0;
    }
    if (0 == nValid) {
        return setSingleUsage(pstUsage, USAGE_ACCEPT);
    }
    pstCommand->eKind = CMD_ACCEPT;
    pstCommand->lNumber = lValue;
    pstCommand->nHasNumber = 1;
    return PARSE_OK;
}

/***********************************************
 * @Name: classifySail
 * @Def: Validates "SAIL <island>": one nonempty token, no
 *       reachability/state check (Phase 1 is syntax-only).
 * @Arg: In: appsTokens = borrowed line tokens.
 *       In: nTokenCount = number of tokens.
 *       Out: pstCommand = receives kind and an owned island copy.
 *       Out: pstUsage = receives the usage line on syntax failure.
 * @Ret: PARSE_OK, PARSE_USAGE, or PARSE_ERROR if the argument copy
 *       could not be allocated.
 ***********************************************/
static eParseStatus classifySail(char **appsTokens, int nTokenCount, tParsedCommand *pstCommand,
                                  tUsageMessage *pstUsage) {
    if (2 != nTokenCount) {
        return setSingleUsage(pstUsage, USAGE_SAIL);
    }
    pstCommand->psArg1 = duplicateString(appsTokens[1]);
    if (NULL == pstCommand->psArg1) {
        return PARSE_ERROR;
    }
    pstCommand->eKind = CMD_SAIL;
    return PARSE_OK;
}

/***********************************************
 * @Name: classifyBuySell
 * @Def: Validates "BUY/SELL <product> <amount>": nonempty product,
 *       positive digit-only amount. For BUY specifically, a literal
 *       "MAP" product must carry amount 1; SELL has
 *       no such restriction.
 * @Arg: In: appsTokens = borrowed line tokens.
 *       In: nTokenCount = number of tokens.
 *       In: eKind = CMD_BUY or CMD_SELL.
 *       In: psUsageText = usage literal for this verb.
 *       Out: pstCommand = receives kind, owned product copy, amount.
 *       Out: pstUsage = receives the usage line on syntax failure.
 * @Ret: PARSE_OK, PARSE_USAGE, or PARSE_ERROR if the product copy could
 *       not be allocated.
 ***********************************************/
static eParseStatus classifyBuySell(char **appsTokens, int nTokenCount, eCommandKind eKind,
                                     const char *psUsageText, tParsedCommand *pstCommand,
                                     tUsageMessage *pstUsage) {
    long lAmount = 0;
    int nValid = 0;

    nValid = (3 == nTokenCount) && (NOSTOS_OK == parseDigitsToLong(appsTokens[2], &lAmount));
    if (nValid && (1 > lAmount || INT_MAX < lAmount)) {
        nValid = 0;
    }
    if (nValid && CMD_BUY == eKind && 0 == strcasecmp("MAP", appsTokens[1]) && 1 != lAmount) {
        nValid = 0;
    }
    if (0 == nValid) {
        return setSingleUsage(pstUsage, psUsageText);
    }
    pstCommand->psArg1 = duplicateString(appsTokens[1]);
    if (NULL == pstCommand->psArg1) {
        return PARSE_ERROR;
    }
    pstCommand->eKind = eKind;
    pstCommand->lNumber = lAmount;
    pstCommand->nHasNumber = 1;
    return PARSE_OK;
}

/***********************************************
 * @Name: dispatchVerb
 * @Def: Looks up the exact (case-insensitive) verb and calls its
 *       classifier. An unmatched verb is PARSE_UNKNOWN, never a
 *       prefix match (e.g. "MAPS" does not match "MAP").
 * @Arg: In: appsTokens = borrowed line tokens (at least one).
 *       In: nTokenCount = number of tokens.
 *       Out: pstCommand = filled by the matching classifier.
 *       Out: pstUsage = filled by the matching classifier.
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
 * @Def: Classifies and validates one complete terminal line (without
 *       its trailing newline) against the eleven Phase 1 commands.
 * @Arg: In: psLine = the line to parse; not modified.
 *       Out: pstCommand = recognized kind/arguments on PARSE_OK; its
 *            owned psArg1 must be released with destroyParsedCommand()
 *            (always safe to call after this function returns).
 *       Out: pstUsage = one or two borrowed usage lines on PARSE_USAGE.
 * @Ret: PARSE_OK, PARSE_UNKNOWN, PARSE_USAGE, PARSE_EMPTY, or
 *       PARSE_ERROR on memory allocation failure.
 ***********************************************/
eParseStatus parseCommand(const char *psLine, tParsedCommand *pstCommand, tUsageMessage *pstUsage) {
    char *psMutable = NULL;
    char **appsTokens = NULL;
    int nTokenCount = 0;
    eParseStatus eResult = PARSE_ERROR;

    pstCommand->eKind = CMD_UNKNOWN;
    pstCommand->psArg1 = NULL;
    pstCommand->lNumber = 0;
    pstCommand->nHasNumber = 0;
    pstUsage->nLineCount = 0;

    /* strtok_r mutates in place; parse a private copy so the caller's
     * line is never modified. */
    psMutable = duplicateString(psLine);
    if (NULL == psMutable) {
        return PARSE_ERROR;
    }
    if (NOSTOS_OK != tokenizeLine(psMutable, &appsTokens, &nTokenCount)) {
        free(psMutable);
        return PARSE_ERROR;
    }
    if (0 == nTokenCount) {
        eResult = PARSE_EMPTY;
    } else {
        eResult = dispatchVerb(appsTokens, nTokenCount, pstCommand, pstUsage);
    }
    free(appsTokens);
    free(psMutable);
    return eResult;
}

/***********************************************
 * @Name: destroyParsedCommand
 * @Def: Frees the owned argument copy of a parsed command, if any.
 * @Arg: In/Out: pstCommand = command to release; psArg1 reset to NULL.
 * @Ret: None.
 ***********************************************/
void destroyParsedCommand(tParsedCommand *pstCommand) {
    free(pstCommand->psArg1);
    pstCommand->psArg1 = NULL;
}

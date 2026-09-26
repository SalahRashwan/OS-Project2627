#ifndef NOSTOS_TYPES_H
#define NOSTOS_TYPES_H

/*
 * @File: types.h
 * @Purpose: Shared, owned data structures for the three Nostos Phase 1
 *           processes. No mutable globals are declared here; every
 *           instance lives in the entry point's own local state.
 * @Author: Salah Ahmed Salaheldin Adly Rashwan
 * @Date: 2026-09-21
 */

/* Fixed on-disk name width for a stock record, per the statement's binary
 * layout (char name[100]). The raw bytes are NOT guaranteed to contain a
 * terminating NUL; see stock.c for the safe display-copy helper. */
#define STOCK_NAME_SIZE 100

/* One network endpoint (IP text + port). Embedded by value in whichever
 * struct owns it; psIp is heap-allocated and owned by that struct. */
typedef struct {
    char *psIp;
    int nPort;
} tEndpoint;

/* One food supply entry inside an Odysseus configuration. */
typedef struct {
    char *psProduct;
    int nAmountKg;
} tFoodEntry;

/* Complete, owned Odysseus configuration (odysseus.dat). */
typedef struct {
    char *psName;
    char *psStorageFolder;
    tEndpoint stIthaca;
    char *psInitialIsland;
    tEndpoint stInitialIslandEndpoint;
    int nGold;
    int nFoodCount;
    tFoodEntry *pstFoods;
} tOdysseusConfig;

/* Complete, owned Ithaca configuration (ithaca.dat). */
typedef struct {
    char *psName;
    char *psMissionFolder;
    tEndpoint stListen;
} tIthacaConfig;

/* One voyage record loaded from voyages.dat, with an internally assigned
 * identifier (the file itself carries no identifier column). */
typedef struct {
    int nId;
    char *psObject;
    char *psFilePath;
    char *psDestination;
    int nReward;
} tVoyage;

/* Dynamically grown array of voyages, owned as a whole by Ithaca.c. */
typedef struct {
    tVoyage *pstVoyages;
    int nCount;
    int nCapacity;
} tVoyageList;

/* One maritime route: a destination island name plus the endpoint of its
 * Island process. Used for both the raw (unfiltered) candidate list read
 * from island.dat and the valid list kept after Sphragis filtering. */
typedef struct {
    char *psDestination;
    char *psIp;
    int nPort;
} tRoute;

/* Dynamically grown array of routes. */
typedef struct {
    tRoute *pstRoutes;
    int nCount;
    int nCapacity;
} tRouteList;

/* Complete, owned Island configuration (island.dat), holding only the
 * post-Sphragis valid route list. */
typedef struct {
    char *psName;
    char *psStorageFolder;
    tEndpoint stListen;
    int nCapacity;
    tRouteList stRoutes;
} tIslandConfig;

/* One binary stock record exactly as laid out on disk: 100 raw name
 * bytes (not necessarily NUL-terminated), then amount, then price. */
typedef struct {
    char sName[STOCK_NAME_SIZE];
    int nAmount;
    int nPrice;
} tStockRecord;

/* Dynamically grown array of stock records. */
typedef struct {
    tStockRecord *pstRecords;
    int nCount;
    int nCapacity;
} tStockList;

/* Recognized command verbs. CMD_UNKNOWN and CMD_EMPTY are internal parser
 * outcomes; they never reach a "successful" print path. */
typedef enum {
    CMD_CONNECT_ITHACA,
    CMD_LIST_VOYAGES,
    CMD_LIST_MARKET,
    CMD_ACCEPT,
    CMD_SAIL,
    CMD_MAP,
    CMD_BUY,
    CMD_SELL,
    CMD_STATUS,
    CMD_DELIVER,
    CMD_CLAIM,
    CMD_UNKNOWN,
    CMD_EMPTY
} eCommandKind;

/* Overall parse result: what the terminal loop must print. */
typedef enum {
    PARSE_OK,
    PARSE_UNKNOWN,
    PARSE_USAGE,
    PARSE_EMPTY
} eParseStatus;

/* One parsed command line. psArg1 is an owned copy (product/island token)
 * valid only for the current terminal iteration; destroyParsedCommand()
 * releases it immediately after the result is printed. */
typedef struct {
    eCommandKind eKind;
    char *psArg1;
    long lNumber;
    int bHasNumber;
} tParsedCommand;

/* Up to two usage lines to print (LIST's ambiguous-subcommand case needs
 * both; every other usage error needs exactly one). The strings are
 * borrowed string literals from commands.h; nothing here is freed. */
typedef struct {
    const char *apsLines[2];
    int nLineCount;
} tUsageMessage;

#endif

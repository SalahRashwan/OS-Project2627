/*
 * @File: stock.c
 * @Purpose: Loader/destructor for an island's binary stock.db file.
 *           Records are decoded field-by-field from raw bytes rather
 *           than read as a native struct, so no padding/alignment
 *           assumption about the compiler's struct layout is needed.
 * @Author: Salah Ahmed Salaheldin Adly Rashwan
 * @Date: 2026-09-21
 */

/* System */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Own */
#include "stock.h"
#include "io.h"
#include "status.h"

/***********************************************
 * @Name: initStockList
 * @Def: Resets a stock list to a safe, destroyable empty state.
 * @Arg: Out: pstList.
 * @Ret: None.
 ***********************************************/
static void initStockList(tStockList *pstList) {
    pstList->pstRecords = NULL;
    pstList->nCount = 0;
    pstList->nCapacity = 0;
}

/***********************************************
 * @Name: destroyStockList
 * @Def: See stock.h.
 ***********************************************/
void destroyStockList(tStockList *pstList) {
    free(pstList->pstRecords);
    initStockList(pstList);
}

/***********************************************
 * @Name: stockRecordNameToString
 * @Def: See stock.h.
 ***********************************************/
char *stockRecordNameToString(const tStockRecord *pstRecord) {
    size_t nLen = 0;
    char *psCopy = NULL;

    while (nLen < STOCK_NAME_SIZE && '\0' != pstRecord->sName[nLen]) {
        nLen++;
    }
    psCopy = malloc(nLen + 1);
    if (NULL == psCopy) {
        return NULL;
    }
    memcpy(psCopy, pstRecord->sName, nLen);
    psCopy[nLen] = '\0';
    return psCopy;
}

/***********************************************
 * @Name: decodeInt32LE
 * @Def: Decodes four bytes at a pointer as a little-endian signed
 *       32-bit integer, independent of host struct layout/endianness.
 * @Arg: In: pBytes = pointer to at least 4 bytes.
 * @Ret: The decoded value.
 ***********************************************/
static int32_t decodeInt32LE(const unsigned char *pBytes) {
    uint32_t wValue = 0;

    wValue = (uint32_t) pBytes[0];
    wValue |= (uint32_t) pBytes[1] << 8;
    wValue |= (uint32_t) pBytes[2] << 16;
    wValue |= (uint32_t) pBytes[3] << 24;
    return (int32_t) wValue;
}

/***********************************************
 * @Name: decodeStockRecord
 * @Def: Fills a stock record from one raw 108-byte on-disk record.
 * @Arg: In: pRaw = 108 raw bytes.
 *       Out: pstRecord = filled record.
 * @Ret: None.
 ***********************************************/
static void decodeStockRecord(const unsigned char *pRaw, tStockRecord *pstRecord) {
    memcpy(pstRecord->sName, pRaw, STOCK_NAME_SIZE);
    pstRecord->nAmount = decodeInt32LE(pRaw + STOCK_AMOUNT_OFFSET);
    pstRecord->nPrice = decodeInt32LE(pRaw + STOCK_PRICE_OFFSET);
}

/***********************************************
 * @Name: readFullRecord
 * @Def: Fills exactly STOCK_RECORD_SIZE bytes with repeated reads,
 *       distinguishing a clean end of file from a truncated record.
 * @Arg: In: nFd = source descriptor.
 *       Out: pRaw = buffer of at least STOCK_RECORD_SIZE bytes.
 * @Ret: 1 = full record read, 0 = clean EOF (no bytes read at all),
 *       NOSTOS_ERROR = truncated record or read failure.
 ***********************************************/
static int readFullRecord(int nFd, unsigned char *pRaw) {
    size_t nTotal = 0;
    ssize_t nRead = 0;

    while (nTotal < STOCK_RECORD_SIZE) {
        nRead = safeRead(nFd, pRaw + nTotal, STOCK_RECORD_SIZE - nTotal);
        if (nRead < 0) {
            return NOSTOS_ERROR;
        }
        if (0 == nRead) {
            if (0 == nTotal) {
                return 0;
            }
            return NOSTOS_ERROR;
        }
        nTotal += (size_t) nRead;
    }
    return 1;
}

/***********************************************
 * @Name: stockListAppend
 * @Def: Appends one already-decoded record to a growable stock list.
 * @Arg: In/Out: pstList = list to grow.
 *       In: pstRecord = record to copy into the list.
 * @Ret: NOSTOS_OK / NOSTOS_ERROR.
 ***********************************************/
static int stockListAppend(tStockList *pstList, const tStockRecord *pstRecord) {
    tStockRecord *pTemp = NULL;
    int nNewCapacity = 0;

    if (pstList->nCount == pstList->nCapacity) {
        if (0 == pstList->nCapacity) {
            nNewCapacity = 8;
        } else {
            nNewCapacity = pstList->nCapacity * 2;
        }
        pTemp = realloc(pstList->pstRecords, (size_t) nNewCapacity * sizeof(tStockRecord));
        if (NULL == pTemp) {
            return NOSTOS_ERROR;
        }
        pstList->pstRecords = pTemp;
        pstList->nCapacity = nNewCapacity;
    }
    pstList->pstRecords[pstList->nCount] = *pstRecord;
    pstList->nCount++;
    return NOSTOS_OK;
}

/***********************************************
 * @Name: loadStockList
 * @Def: See stock.h.
 ***********************************************/
int loadStockList(const char *psPath, tStockList *pstList) {
    int nFd = -1;
    unsigned char acRaw[STOCK_RECORD_SIZE];
    int nReadResult = 0;
    tStockRecord stRecord;

    initStockList(pstList);
    nFd = safeOpenReadOnly(psPath);
    if (-1 == nFd) {
        return NOSTOS_ERROR;
    }
    for (;;) {
        nReadResult = readFullRecord(nFd, acRaw);
        if (0 == nReadResult) {
            close(nFd);
            return NOSTOS_OK;
        }
        if (NOSTOS_ERROR == nReadResult) {
            close(nFd);
            destroyStockList(pstList);
            return NOSTOS_ERROR;
        }
        decodeStockRecord(acRaw, &stRecord);
        if (NOSTOS_OK != stockListAppend(pstList, &stRecord)) {
            close(nFd);
            destroyStockList(pstList);
            return NOSTOS_ERROR;
        }
    }
}

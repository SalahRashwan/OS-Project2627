#ifndef NOSTOS_STOCK_H
#define NOSTOS_STOCK_H

/*
 * @File: stock.h
 * @Purpose: Loader/destructor for an island's binary stock.db file, and
 *           a safe display-copy helper for its fixed, not necessarily
 *           NUL-terminated 100-byte name field.
 * @Author: Salah Ahmed Salaheldin Adly Rashwan
 * @Date: 2026-09-21
 */

/* System Includes */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Project Includes */
#include "types.h"

/* On-disk record layout: 100 name bytes, then two little-endian signed
 * 32-bit integers (amount, price), 108 bytes total. */
#define STOCK_RECORD_SIZE 108
#define STOCK_AMOUNT_OFFSET 100
#define STOCK_PRICE_OFFSET 104

/***********************************************
 * @Name: loadStockList
 * @Def: Opens, decodes, and closes a binary stock.db file, reading
 *       fixed-size records until a clean EOF. A partial final record
 *       is reported as a truncated-file error, never counted as a
 *       complete product.
 * @Arg: In: psPath = path to the stock file.
 *       Out: pstList = filled on success; safe to destroy on failure.
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR otherwise.
 ***********************************************/
int loadStockList(const char *psPath, tStockList *pstList);

/***********************************************
 * @Name: destroyStockList
 * @Def: Frees the stock record array (records own no nested pointers).
 * @Arg: In/Out: pstList = list to release.
 * @Ret: None.
 ***********************************************/
void destroyStockList(tStockList *pstList);

/***********************************************
 * @Name: stockRecordNameToString
 * @Def: Produces a newly owned, guaranteed NUL-terminated copy of a
 *       stock record's raw 100-byte name field, stopping at the first
 *       embedded NUL if one is present within the field.
 * @Arg: In: pstRecord = record whose name field is copied.
 * @Ret: A newly owned string, or NULL on allocation failure.
 ***********************************************/
char *stockRecordNameToString(const tStockRecord *pstRecord);

#endif

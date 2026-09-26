#ifndef NOSTOS_VOYAGES_H
#define NOSTOS_VOYAGES_H

/*
 * @File: voyages.h
 * @Purpose: Loader and destructor for Ithaca's voyages.dat file. IDs are
 *           not present on disk; they are assigned internally in file
 *           order starting at 1 (assumption A04).
 * @Author: Salah Ahmed Salaheldin Adly Rashwan
 * @Date: 2026-09-21
 */

/* Own */
#include "types.h"

/***********************************************
 * @Name: loadVoyages
 * @Def: Opens, parses, and closes a voyages.dat file. Blank lines are
 *       skipped; any line with a token count other than 0 or 4 is a
 *       malformed-file error.
 * @Arg: In: psPath = path to the voyages file.
 *       Out: pstList = filled on success; safe to destroy on failure.
 * @Ret: NOSTOS_OK on success, NOSTOS_ERROR otherwise.
 ***********************************************/
int loadVoyages(const char *psPath, tVoyageList *pstList);

/***********************************************
 * @Name: destroyVoyageList
 * @Def: Frees every owned string in every voyage and the array itself.
 * @Arg: In/Out: pstList = list to release.
 * @Ret: None.
 ***********************************************/
void destroyVoyageList(tVoyageList *pstList);

#endif

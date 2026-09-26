#ifndef NOSTOS_STATUS_H
#define NOSTOS_STATUS_H

/*
 * @File: status.h
 * @Purpose: Shared success/failure return codes used by every loader and
 *           adapter function in the project, so callers can propagate a
 *           single consistent convention instead of inventing one per file.
 * @Author: Salah Ahmed Salaheldin Adly Rashwan
 * @Date: 2026-09-21
 */

/* Generic status codes returned by loaders/helpers that do not need a
 * richer result (the Sphragis-derived route count is the one exception:
 * that function keeps returning its own nonnegative count / negative
 * SPHRAGIS_ERROR_* codes, per the vendor contract). */
#define NOSTOS_OK 0
#define NOSTOS_ERROR (-1)

/* Process exit-status convention (assumption A21; not an official
 * requirement, but documented and used consistently by all three
 * entry points). */
#define NOSTOS_EXIT_OK 0
#define NOSTOS_EXIT_ARGS 1
#define NOSTOS_EXIT_IO 2

#endif

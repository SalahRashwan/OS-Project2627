/* 
 * SPHRAGIS Library for OS 26-27
 * 
 * Made by: Valeria, Aaron, Victor
 *
 */

#ifndef SPHRAGIS_H
#define SPHRAGIS_H

#define SPHRAGIS_OK 0
#define SPHRAGIS_ERROR -1
#define SPHRAGIS_ERROR_INVALID_ISLAND -2
#define SPHRAGIS_ERROR_INVALID_CONNECTION -3

/* Valid names (case-insensitive):
 * Aeaea, Ogygia, Scheria, Thrinacia, Aeolia, Ismarus. */

/* One discovered island and its known destinations.
 * name:               the island's name.
 * known_islands:      array of destination names, or NULL when empty.
 * known_island_count: the number of entries in known_islands.
 */
typedef struct {
    char *name;
    char **known_islands;
    int known_island_count;
} SPHRAGIS_Island;

/***********************************************
 * @Purpose: Adds an island's glyph to a PNG or JPEG image.
 * @Parameters: in: island_name = name of the island whose glyph is added.
 *              in: input_path = path to the original PNG or JPEG image.
 *              in: output_path = path where the signed image is written.
 * @Return: SPHRAGIS_OK on success, SPHRAGIS_ERROR_INVALID_ISLAND for an
 *          unknown island, or SPHRAGIS_ERROR on an image read/write failure.
 ***********************************************/
int SPHRAGIS_sign_with_glyph(char *island_name, char *input_path, char *output_path);

/***********************************************
 * @Purpose: Checks whether an image contains an island's glyph.
 * @Parameters: in: island_name = name of the island to check for.
 *              in: image_path = path to the PNG or JPEG image to examine.
 * @Return: 1 if present, 0 if absent, SPHRAGIS_ERROR_INVALID_ISLAND for an
 *          unknown island, or SPHRAGIS_ERROR if the image cannot be read.
 ***********************************************/
int SPHRAGIS_has_glyph(char *island_name, char *image_path);

/***********************************************
 * @Purpose: Identifies the island whose glyph appears in an image.
 * @Parameters: in: image_path = path to the PNG or JPEG image to examine.
 * @Return: The island name, or NULL on a read failure or an unrecognized
 *          glyph. The returned string must not be modified or freed.
 ***********************************************/
char *SPHRAGIS_identify_glyph(char *image_path);

/***********************************************
 * @Purpose: Removes disconnected destinations and updates the route count.
 * @Parameters: in/out: island = configuration with separately heap-allocated
 *              destination names. Rejected names are freed; survivors keep
 *              their order and unused slots become NULL. The caller owns
 *              the remaining names and the known_islands array.
 * @Return: The new count, SPHRAGIS_ERROR_INVALID_ISLAND for an invalid island,
 *          or SPHRAGIS_ERROR_INVALID_CONNECTION for an invalid array/count.
 ***********************************************/
int SPHRAGIS_filter_island_configuration(SPHRAGIS_Island *island);

/***********************************************
 * @Purpose: Displays the discovered islands and routes in the terminal.
 * @Parameters: in: known_islands = island array, or NULL when empty. Aeaea
 *              must be first; every named destination needs its own entry.
 *              in: island_count = number of array entries (0 to 6).
 * @Return: SPHRAGIS_OK on success, SPHRAGIS_ERROR_INVALID_ISLAND for invalid
 *          or duplicate islands, SPHRAGIS_ERROR_INVALID_CONNECTION for invalid
 *          routes, or SPHRAGIS_ERROR if the terminal has fewer than 80 columns.
 ***********************************************/
int SPHRAGIS_display_map(SPHRAGIS_Island *known_islands, int island_count);

#endif

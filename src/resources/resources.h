#ifndef RESOURCES_H
#define RESOURCES_H

#include <stdbool.h>
#include <stdint.h>

#include "resources/data/usa/general-en_US.csv.h"
#include "resources/data/usa/nsa-en_US.csv.h"
#include "resources/data/ignores.txt.h"

#define RESOURCES_DATA_SOURCES 2

#define COUNTRY_NO_MATCH 0
#define COUNTRY_USA 1
#define COUNTRY_USA_NSA 2

// Read a file (does not follow symlinks)
char *resources_loadFile(const char *filePath) __attribute__((nonnull(1)));

void resources_countWord(const char *word, size_t *occurances);
uint8_t resources_bestMatch(size_t *occurances);

#endif

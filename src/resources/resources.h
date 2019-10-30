#ifndef RESOURCES_H
#define RESOURCES_H

#include "resources/data/usa/general-en_US.csv.h"
#include "resources/data/usa/nsa-en_US.csv.h"

// Read a file (does not follow symlinks)
char *resources_loadFile(const char *filePath) __attribute__((nonnull(1)));

#endif

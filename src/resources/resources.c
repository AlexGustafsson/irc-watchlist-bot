#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "resources.h"

char *resources_loadFile(const char *filePath) {
  // Open the file in read mode and fail if the path is a directory
  FILE *file = fopen(filePath, "r+");
  if (file == 0)
    return 0;

  // Seek to the end of file to get file size
  if (fseek(file, 0L, SEEK_END) != 0) {
    fclose(file);
    return 0;
  }

  // Get the actual file size
  ssize_t fileSize = ftell(file);
  if (fileSize == -1) {
    fclose(file);
    return 0;
  }

  // Go back to the start of the file in order to read it
  rewind(file);

  char *buffer = (char *)malloc(fileSize + 1);

  // Read the file, character by character until EOF
  char current = 0;
  size_t currentIndex = 0;
  while ((current = fgetc(file)) != EOF)
    buffer[currentIndex++] = current;
  // Null-terminate
  buffer[fileSize] = 0;

  fclose(file);
  return buffer;
}

void resources_countWord(const char *word, size_t *occurances) {
  for (size_t i = 0; RESOURCES_USA_GENERAL_EN_US[i] != 0; i++) {
    if (strcasecmp(word, RESOURCES_USA_GENERAL_EN_US[i]) == 0) {
      printf("%zu: %s\n", i, RESOURCES_USA_GENERAL_EN_US[i]);
      occurances[0]++;
      break;
    }
  }

  for (size_t i = 0; RESOURCES_USA_NSA_EN_US[i] != 0; i++) {
    if (strcasecmp(word, RESOURCES_USA_NSA_EN_US[i]) == 0) {
      printf("%zu: %s\n", i, RESOURCES_USA_NSA_EN_US[i]);
      occurances[1]++;
      break;
    }
  }
}

uint8_t resources_bestMatch(size_t *occurances) {
  ssize_t bestIndex = -1;
  size_t bestOccurances = 0;
  for (size_t i = 0; i < RESOURCES_DATA_SOURCES; i++) {
    printf("%zu: %zu\n", i, occurances[i]);
    if (occurances[i] > bestOccurances) {
      bestOccurances = occurances[i];
      bestIndex = i;
    }
  }

  if (bestIndex == 0)
    return COUNTRY_USA;
  else if (bestIndex == 1)
    return COUNTRY_USA;

  return COUNTRY_NO_MATCH;
}

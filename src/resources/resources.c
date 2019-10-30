#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

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

#include <stdint.h>
#include <stdio.h>

#include "resources/resources.h"

int main(int argc, const char *argv[]) {
  for (size_t i = 0; i < src_resources_data_usa_general_en_US_csv_len && i < 100; i++) {
    printf("%c", src_resources_data_usa_general_en_US_csv[i]);
  }
}

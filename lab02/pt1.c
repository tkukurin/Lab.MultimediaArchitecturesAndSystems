#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "common.c"
#include "pgm.c"

#define GROUP_COUNT 16
#define BYTE1_MASK 0b11110000
#define BYTE2_MASK 0b1111000000000000

int main(int argc, const char **argv) {
  Image *image = load_pgm("lenna.pgm");
  int total = image->width * image->height;
  int groups[GROUP_COUNT] = {0};
  for (int i = 0; i < total; i++) {
    Grayscale byte = image->data[i];
    int group = (byte & BYTE1_MASK) >> 4;
    if (group < 0 || group >= GROUP_COUNT) {
      fprintf(stderr, "Invalid group: %d\n", group);
      break;
    }
    groups[group]++;
  }

  IMG_FREE(image);

  for (int i = 0; i < GROUP_COUNT; i++) {
    printf("[%d] -> [%d]\n", i, groups[i]);
  }

  return 0;
}


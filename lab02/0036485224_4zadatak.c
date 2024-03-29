#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "common.c"
#include "pgm.c"

#define LENNA0_FNAME "lenna.pgm"
#define LENNA1_FNAME "lenna1.pgm"

#define BLOCK_SIZE 16
#define DELTA_SEARCH 16

Grayscale *make_block(Image *img, int x0, int y0, int blocksize, Grayscale *block) {
  Grayscale *data = img->data;
  block = block != NULL ? block : malloc(sizeof(Grayscale) * blocksize * blocksize);

  for (int y = y0; y < y0 + blocksize; y++) {
    for (int x = x0; x < x0 + blocksize; x++) {
      block[(y - y0) * blocksize + (x - x0)] = data[y * img->width + x];
    }
  }

  return block;
}

double mean_avg_disturbance(Grayscale *block1, Grayscale *block2, int n_elements) {
  int sum = 0;
  for (int i = 0; i < n_elements; i++) {
    sum += fabs(block1[i] - block2[i]);
  }

  return (double) sum / n_elements;
}

typedef struct { int x, y; double delta; } Point;
Point find_move_vector(Image *fst, Image *snd, Point start) {
  int startx = start.x;
  int starty = start.y;
  Grayscale *reference = make_block(fst, startx, starty, BLOCK_SIZE, NULL);
  Grayscale *block = make_block(snd, startx, starty, BLOCK_SIZE, NULL);

  Point disturbance = { .delta = 999999 };
  for (int y = fmax(0, starty - DELTA_SEARCH); y < fmin(snd->height - DELTA_SEARCH, starty + DELTA_SEARCH + 1); y++) {
    for (int x = fmax(0, startx - DELTA_SEARCH); x < fmin(snd->width - DELTA_SEARCH, startx + DELTA_SEARCH + 1); x++) {
      block = make_block(snd, x, y, BLOCK_SIZE, block);
      double delta = mean_avg_disturbance(block, reference, BLOCK_SIZE * BLOCK_SIZE);
      if (delta < disturbance.delta) {
        disturbance.x = x - startx;
        disturbance.y = y - starty;
        disturbance.delta = delta;
      }
    }
  }

  free(block);
  free(reference);
  return disturbance;
}

Point calc_startpt(Image *image, int block_nr) {
  int w = image->width;
  int line_blocks = image->width / BLOCK_SIZE;
  return (Point) { 
    .x = (block_nr % (w / BLOCK_SIZE)) * BLOCK_SIZE, 
    .y = (block_nr / (w / BLOCK_SIZE)) * BLOCK_SIZE
  };
}

int main(int argc, const char **argv) {
  _assert(argc == 2, "Expect block 0-based ordinal number as argument.");
  Image *first = load_pgm(LENNA1_FNAME);
  Image *second = load_pgm(LENNA0_FNAME);

  Point start = calc_startpt(first, atoi(argv[1]));
  Point solution = find_move_vector(first, second, start);
  printf("(%d,%d)\n", solution.x, solution.y);

  IMG_FREE(first);
  IMG_FREE(second);
  return 0;
}


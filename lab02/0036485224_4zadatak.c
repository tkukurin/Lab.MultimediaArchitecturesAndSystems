#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "common.c"

#define LENNA0_FNAME "lenna.pgm"
#define LENNA1_FNAME "lenna1.pgm"

Header pgm_header(FILE *file) {
  String *token = malloc(sizeof(String));
  Header header;

  for (State state = S_INIT; state <= S_MAX_COLOR; state++) {
    token->len = fscanf(file, "%s", token->buf);
    while (token->len > 0 && token->buf[0] == COMMENT) {
      skip_line(file);
      token->len = fscanf(file, "%s", token->buf);
    }

    if (state == S_INIT) {
      strcpy(header.ppm_id, token->buf);
    } else if (state == S_WIDTH) {
      header.width = atoi(token->buf);
    } else if (state == S_HEIGHT) {
      header.height = atoi(token->buf);
    } else {
      header.max_grayscale = atoi(token->buf);
    }
  }
  
  free(token);
  return header;
}

Image *from_pgm_p5(FILE *file, Header header) {
  int total = header.width * header.height;
  Image *image = malloc(sizeof(Image));
  image->width = header.width;
  image->height = header.height;
  size_t intensity_size = header.max_grayscale < 256 ? 1 : 2;
  image->data = malloc(total * intensity_size);
  size_t nread = fread(image->data, intensity_size, total, file);
  if (nread != total) {
    fprintf(stderr, "Read %d bytes out of expected %d.\n", nread, total);
    IMG_FREE(image);
    return NULL;
  }
  return image;
}

Image *load_pgm(char *fname) {
  FILE *pgm_file = fopen(fname, "r");
  _assert(pgm_file != NULL, "Error opening input file.");

  Header header = pgm_header(pgm_file);
  fprintf(stderr, "[%s] Loaded header for %dx%d image with %d max intensity.\n",
      fname, header.width, header.height, header.max_grayscale);
  _assert(strcmp(header.ppm_id, "P5") == 0, "Expect P5 type.");
  _assert(header.max_grayscale >= 0 && header.max_grayscale < 65536, 
      "Expect valid max grayscale value.");
  if (getc(pgm_file) == '\r') {
    getc(pgm_file);
  }

  Image *image = from_pgm_p5(pgm_file, header);
  _assert(image != NULL, "Error reading first Lenna image.");
  fclose(pgm_file);

  fprintf(stderr, "[%s] Loaded image.\n", fname);
  return image;
}

Grayscale *make_block(Grayscale *data, int x0, int y0, int blocksize, Grayscale *block) {
  block = block != NULL ? block : malloc(sizeof(Grayscale) * blocksize * blocksize);
  for (int x = x0; x < x0 + blocksize; x++) {
    for (int y = y0; y < y0 + blocksize; y++) {
      block[y - y0][x - x0] = data[y][x];
    }
  }
  return block;
}

#define BLOCK_SIZE 16
#define DELTA_SEARCH 16

double mean_avg_disturbance(Grayscale *block1, Grayscale *block2, int n_elements) {
  int sum = 0;
  for (int i = 0; i < n_elements; i++) {
    sum += abs(block1[i] - block2[i]);
  }

  return (double) sum / n_elements;
}

bool in_bounds(int x, int y, Image* image) {
  return x >= 0 && y >= 0 && x < image->width && y < image->height;
}

typedef struct { int x, y; int delta; } Point;
Point find_move_vector(Image *fst, Image *snd, int startx, int starty) {
  Point disturbance = { .disturbance = 999999 };
  Grayscale *reference = make_block(fst->data, startx, starty, BLOCK_SIZE, NULL);
  Grayscale *block = make_block(snd->data, startx, starty, BLOCK_SIZE, NULL);

  for (int y = max(0, starty - DELTA_SEARCH); y < min(snd->height, starty + DELTA_SEARCH); y++) {
    for (int x = max(0, startx - DELTA_SEARCH); x < min(snd->width, startx + DELTA_SEARCH); x++) {
      block = make_block(snd->data, x, y, BLOCK_SIZE, block);
      int delta = mean_avg_disturbance(block, reference, BLOCK_SIZE);
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

int main(int argc, const char **argv) {
  _assert(argc == 2, "Expect block 0-based ordinal number as argument.");
  Image *first = load_pgm(LENNA0_FNAME);
  Image *second = load_pgm(LENNA1_FNAME);

  Point solution = find_move_vector(first, second);
  printf("(%d,%d)\n", solution.x, solution.y);

  IMG_FREE(first);
  IMG_FREE(second);
  return 0;
}


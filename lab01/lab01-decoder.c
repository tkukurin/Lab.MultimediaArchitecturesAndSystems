#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "common.c"

#define IMG_FREE(i) \
  if((i)->data) \
    free((i)->data); \
  free(i);

int to_1d_index(int index, int width, int block_size, int off_x) {
  return off_x + (index / block_size) * width + index % block_size;
}

void in_zigzag(FILE *file, ImageYCbCr *image, int m, int off_x, char mode) {
  for (int i = 0; i < m * 2; i++) {
    for (int j = (i < m) ? 0 : i - m + 1; j <= i && j < m; j++) {
      int loc = (i & 1) 
        ? j * (m - 1) + i 
        : (i - j) * m + j;
      int idx = to_1d_index(loc, image->width, m, off_x);
      double *data = mode == 'y' 
        ? &image->data[idx].y 
        : mode == 'r' 
          ? &image->data[idx].cr 
          : &image->data[idx].cb;
      fscanf(file, "%lf", data);
    }
  }
}

void read_in(FILE *file, ImageYCbCr *image, char mode) {
  int nxblocks = image->width / BLOCK_SIZE;
  int nyblocks = image->height / BLOCK_SIZE;
  for (int block = 0, off_x = 0; block < nxblocks * nyblocks; block++) {
    off_x = (block / nxblocks) * (image->width * BLOCK_SIZE);
    off_x += (block % nxblocks) * BLOCK_SIZE;
    in_zigzag(file, image, BLOCK_SIZE, off_x, mode);
  }
}

ImageYCbCr *from_output_data(FILE *file) {
  ImageYCbCr *image = malloc(sizeof(ImageYCbCr));
  fscanf(file, "%d %d", &image->width, &image->height);
  image->data = malloc(image->width * image->height * sizeof(YCbCr));

  read_in(file, image, 'y');
  read_in(file, image, 'b');
  read_in(file, image, 'r');

  return image;
}


#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
Rgb to_rgb(YCbCr ycbcr) {
  double y = ycbcr.y, cb = ycbcr.cb, cr = ycbcr.cr;
  return (Rgb) {
    .r = MAX(0, MIN(y + 1.402 * (cr - 128), 255)),
    .g = MAX(0, MIN(y - 0.344136 * (cb - 128) - 0.714136 * (cr - 128), 255)),
    .b = MAX(0, MIN(y + 1.772 * (cb - 128), 255))
  };
}

Image *to_image_rgb(ImageYCbCr *image) {
  Image *rgb = malloc(sizeof(Image));
  rgb->width = image->width;
  rgb->height = image->height;
  int total = image->width * image->height;
  rgb->data = malloc(total * sizeof(Rgb));
  for (int i = 0; i < total; i++)
    rgb->data[i] = to_rgb(image->data[i]);
  return rgb;
}

ImageYCbCr *idct(ImageYCbCr *image) {
  ImageYCbCr *result = malloc(sizeof(ImageYCbCr));
  result->width = image->width;
  result->height = image->height;
  result->data = malloc(image->width * image->height * sizeof(YCbCr));

  double kc[2] = { 1.0 / sqrt(2.0), 1.0 };
  int nxblocks = image->width / BLOCK_SIZE;
  int nyblocks = image->height / BLOCK_SIZE;
  for (int block = 0; block < nxblocks * nyblocks; block++) {
    int off_y = (image->width * BLOCK_SIZE) * (block / nxblocks);

    int quant_y[BLOCK_SIZE][BLOCK_SIZE] = {0};
    int quant_cb[BLOCK_SIZE][BLOCK_SIZE] = {0};
    int quant_cr[BLOCK_SIZE][BLOCK_SIZE] = {0};
    for (int off_x = 0, i = 0; i < BLOCK_SIZE; i++, off_x += image->width) {
      int starty = (block % nxblocks) * BLOCK_SIZE + off_y + off_x;
      for (int j = starty, k = 0; j < starty + BLOCK_SIZE; j++, k++) {
        quant_y[i][k] = image->data[j].y;
        quant_cb[i][k] = image->data[j].cb;
        quant_cr[i][k] = image->data[j].cr;
      }
    }

    double dct_y[BLOCK_SIZE][BLOCK_SIZE] = {0};
    double dct_cb[BLOCK_SIZE][BLOCK_SIZE] = {0};
    double dct_cr[BLOCK_SIZE][BLOCK_SIZE] = {0};
    for (int i = 0; i < BLOCK_SIZE; i++) {
      for (int j = 0; j < BLOCK_SIZE; j++) {
        dct_y[i][j]  = round(quant_y[i][j] * k1[i][j]);
        dct_cb[i][j] = round(quant_cb[i][j] * k2[i][j]);
        dct_cr[i][j] = round(quant_cr[i][j] * k2[i][j]);
      }
    }

    double y[BLOCK_SIZE][BLOCK_SIZE] = {0};
    double cb[BLOCK_SIZE][BLOCK_SIZE] = {0};
    double cr[BLOCK_SIZE][BLOCK_SIZE] = {0};
    for (int i = 0; i < BLOCK_SIZE; i++) {
      for (int j = 0; j < BLOCK_SIZE; j++) {
        for (int k = 0; k < BLOCK_SIZE; k++) {
          for (int l = 0; l < BLOCK_SIZE; l++) {
            double cu = kc[k != 0];
            double cv = kc[l != 0];
            double cos_val = 
              cos((2 * i + 1) * k * M_PI / 16.0) 
                * cos((2 * j + 1) * l * M_PI / 16.0) * cu * cv; 
            y[i][j] += dct_y[k][l] * cos_val;
            cb[i][j] += dct_cb[k][l] * cos_val;
            cr[i][j] += dct_cr[k][l] * cos_val;
          }
        }

        y[i][j] *= 0.25;
        cb[i][j] *= 0.25;
        cr[i][j] *= 0.25;
      }
    }

    for (int off_x = 0, i = 0; i < BLOCK_SIZE; i++, off_x += image->width) {
      int starty = (block % nxblocks) * BLOCK_SIZE + off_y + off_x;
      for (int j = starty, k = 0; j < starty + BLOCK_SIZE; j++, k++) {
        result->data[j] = (YCbCr) {
          .y = y[i][k] + 128,
          .cb = cb[i][k] + 128,
          .cr = cr[i][k] +128
        };
      }
    }
  }

  return result;
}

void output_p6(FILE *file, Image *image) {
  fprintf(file, "P6 %d %d 255\n", image->width, image->height);
  fwrite(image->data, sizeof(Rgb), image->width * image->height, file);
}

int main(int argc, const char **argv) {
  FILE *out = fopen(OUT_TXT, "r");
  ImageYCbCr *saved = from_output_data(out);
  _assert(saved->width > 0 && saved->height > 0, "Image dimensions invalid");
  fclose(out);

  ImageYCbCr *image_unquantized = idct(saved);
  IMG_FREE(saved);

  FILE *ppm_file_out = fopen(OUT_PPM, "wb");
  Image *back_to_rgb = to_image_rgb(image_unquantized);
  IMG_FREE(image_unquantized);

  output_p6(ppm_file_out, back_to_rgb);
  fclose(ppm_file_out);

  IMG_FREE(back_to_rgb);
  return 0;
}



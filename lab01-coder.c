#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "common.c"
#include "debug.c"

#define IMG_FREE(i) \
  if((i)->data) \
    free((i)->data); \
  free(i);

Header ppm_header(FILE *file) {
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
      header.max_rgb = atoi(token->buf);
    }
  }
  
  free(token);
  return header;
}

Image *from_ppm_p6(FILE *file, Header header) {
  int total = header.width * header.height;
  Image *image = malloc(sizeof(Image));
  image->width = header.width;
  image->height = header.height;
  image->data = malloc(total * sizeof(Rgb));
  size_t nread = fread(image->data, sizeof(Rgb), total, file);
  if (nread != total) {
    IMG_FREE(image);
    return NULL;
  }
  return image;
}

YCbCr to_ycbcr(Rgb rgb) {
  int R = rgb.r, G = rgb.g, B = rgb.b;
  return (YCbCr) { 
    .y  = 0.299 * R + 0.587 * G + 0.114 * B, 
    .cb = -0.16874 * R - 0.33126 * G + 0.5 * B + 128, 
    .cr =  0.5 * R - 0.41869 * G - 0.08131 * B + 128
  };
}

ImageYCbCr *to_image_ycbcr(Image *image) {
  ImageYCbCr *ycc = malloc(sizeof(ImageYCbCr));
  ycc->width = image->width;
  ycc->height = image->height;
  int total = image->width * image->height;
  ycc->data = malloc(total * sizeof(YCbCr));
  for (int i = 0; i < total; i++)
    ycc->data[i] = to_ycbcr(image->data[i]);
  return ycc;
}

ImageYCbCr *dct(ImageYCbCr *image) {
  ImageYCbCr *result = malloc(sizeof(ImageYCbCr));
  result->width = image->width;
  result->height = image->height;
  result->data = malloc(image->width * image->height * sizeof(YCbCr));

  double kc[2] = { 1.0 / sqrt(2.0), 1.0 };
  int width = image->width;
  int nxblocks = image->width / BLOCK_SIZE;
  int nyblocks = image->height / BLOCK_SIZE;
  int nblocks = nxblocks * nyblocks;

  for (int block = 0; block < nblocks; block++) {
    int off_y = width * BLOCK_SIZE * (block / nxblocks);

    double y[BLOCK_SIZE][BLOCK_SIZE] = {0};
    double cb[BLOCK_SIZE][BLOCK_SIZE] = {0};
    double cr[BLOCK_SIZE][BLOCK_SIZE] = {0};
    for (int off_x = 0, i = 0; i < BLOCK_SIZE; i++, off_x += width) {
      int starty = (block % nxblocks) * BLOCK_SIZE + off_y + off_x;
      int endy = starty + BLOCK_SIZE;
      for (int j = starty, k = 0; j < endy; j++, k++) {
        y[i][k] = image->data[j].y - 128;
        cb[i][k] = image->data[j].cb - 128;
        cr[i][k] = image->data[j].cr - 128;
      }
    }

    double dct_y[BLOCK_SIZE][BLOCK_SIZE] = {0};
    double dct_cb[BLOCK_SIZE][BLOCK_SIZE] = {0};
    double dct_cr[BLOCK_SIZE][BLOCK_SIZE] = {0};
    for (int i = 0; i < BLOCK_SIZE; i++) {
      for (int j = 0; j < BLOCK_SIZE; j++) {
        for (int k = 0; k < BLOCK_SIZE; k++) {
          for (int l = 0; l < BLOCK_SIZE; l++) {
            double cos_val = 
              cos((2 * k + 1) * i * M_PI / 16.0) 
                * cos((2 * l + 1) * j * M_PI / 16.0); 
            dct_y[i][j] += y[k][l] * cos_val;
            dct_cb[i][j] += cb[k][l] * cos_val;
            dct_cr[i][j] += cr[k][l] * cos_val;
          }
        }

        double cu = kc[i != 0];
        double cv = kc[j != 0];
        dct_y[i][j] *= 0.25 * cu * cv;
        dct_cb[i][j] *= 0.25 * cu * cv;
        dct_cr[i][j] *= 0.25 * cu * cv;
      }
    }

    int quant_y[BLOCK_SIZE][BLOCK_SIZE] = {0};
    int quant_cb[BLOCK_SIZE][BLOCK_SIZE] = {0};
    int quant_cr[BLOCK_SIZE][BLOCK_SIZE] = {0};
    for (int i = 0; i < BLOCK_SIZE; i++) {
      for (int j = 0; j < BLOCK_SIZE; j++) {
        quant_y[i][j]  = round(dct_y[i][j] / k1[i][j]);
        quant_cb[i][j] = round(dct_cb[i][j] / k2[i][j]);
        quant_cr[i][j] = round(dct_cr[i][j] / k2[i][j]);
      }
    }

    for (int off_x = 0, i = 0; i < BLOCK_SIZE; i++, off_x += width) {
      int starty = (block % nxblocks) * BLOCK_SIZE + off_y + off_x;
      int endy = starty + BLOCK_SIZE;
      for (int j = starty, k = 0; j < endy; j++, k++) {
        result->data[j] = (YCbCr) {
          .y = quant_y[i][k],
          .cb = quant_cb[i][k],
          .cr = quant_cr[i][k]
        };
      }
    }
  }

  return result;
}

void output(FILE *file, ImageYCbCr *image) {
  fprintf(file, "%d %d\n", image->width, image->height);
  int n_elements = image->width * image->height;
  
  for(int i = 0; i < n_elements; i++)
    fprintf(file, "%d ", (int)image->data[i].y);
  fprintf(file, "\n");

  for(int i = 0; i < n_elements; i++)
    fprintf(file, "%d ", (int)image->data[i].cb);
  fprintf(file, "\n");

  for(int i = 0; i < n_elements; i++)
    fprintf(file, "%d ", (int)image->data[i].cr);
}


int main(int argc, const char **argv) {
  assert(argc == 2, "Expect file name as argument.");
  FILE *ppm_file = fopen(argv[1], "r");
  assert(ppm_file != NULL, "Error opening input file.");

  Header header = ppm_header(ppm_file);
  assert(strcmp(header.ppm_id, "P6") == 0, "Expect P6 PPM type.");
  assert(header.max_rgb == 255, "Expect 255 RGB max");
  fseek(ppm_file, 1, SEEK_CUR);

  Image *image = from_ppm_p6(ppm_file, header);
  assert(image != NULL, "Error reading image.");
  fclose(ppm_file);

  ImageYCbCr *image_y_cb_cr = to_image_ycbcr(image);
  IMG_FREE(image);
  assert(image_y_cb_cr != NULL, "Error creating YCbCr image.");

  ImageYCbCr *image_quantized = dct(image_y_cb_cr);
  IMG_FREE(image_y_cb_cr);
  assert(image_quantized != NULL, "Error creating DCT image.");

  FILE *out_file = fopen(OUT_TXT, "w");
  output(out_file, image_quantized);
  fclose(out_file);
  IMG_FREE(image_quantized);
	return 0;
}


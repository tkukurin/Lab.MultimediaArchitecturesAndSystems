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

ImageYCbCr *from_output_data(FILE *file) {
  ImageYCbCr *image = malloc(sizeof(ImageYCbCr));
  fscanf(file, "%d %d", &image->width, &image->height);
  int total = image->width * image->height;
  image->data = malloc(total * sizeof(YCbCr));

  for (int i = 0; i < total; i++)
     fscanf(file, "%lf", &(image->data[i]).y);

  for (int i = 0; i < total; i++)
     fscanf(file, "%lf", &image->data[i].cb);

  for (int i = 0; i < total; i++)
     fscanf(file, "%lf", &image->data[i].cr);

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

Rgb to_rgb(YCbCr ycbcr) {
  double y = ycbcr.y, cb = ycbcr.cb, cr = ycbcr.cr;
  return (Rgb) {
    .r = y + 1.402 * (cr - 128),
    .g = y - 0.344136 * (cb - 128) - 0.714136 * (cr - 128),
    .b = y + 1.772 * (cb - 128)
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

#define BLOCK_SIZE 8
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

ImageYCbCr *idct(ImageYCbCr *image) {
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

    int quant_y[BLOCK_SIZE][BLOCK_SIZE] = {0};
    int quant_cb[BLOCK_SIZE][BLOCK_SIZE] = {0};
    int quant_cr[BLOCK_SIZE][BLOCK_SIZE] = {0};
    for (int off_x = 0, i = 0; i < BLOCK_SIZE; i++, off_x += width) {
      int starty = (block % nxblocks) * BLOCK_SIZE + off_y + off_x;
      int endy = starty + BLOCK_SIZE;
      for (int j = starty, k = 0; j < endy; j++, k++) {
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

    for (int off_x = 0, i = 0; i < BLOCK_SIZE; i++, off_x += width) {
      int starty = (block % nxblocks) * BLOCK_SIZE + off_y + off_x;
      int endy = starty + BLOCK_SIZE;
      for (int j = starty, k = 0; j < endy; j++, k++) {
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

void output_p6(FILE *file, Image *image) {
  fprintf(file, "P6 %d %d 255\n", image->width, image->height);
  fwrite(image->data, sizeof(Rgb), image->width * image->height, file);
}

void full_example(const char *file_name) {
  FILE *ppm_file = fopen(file_name, "r");
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

  FILE *out_file = fopen("out.txt", "w");
  output(out_file, image_quantized);
  fclose(out_file);
  ImageYCbCr *image_unquantized = idct(image_quantized);
  IMG_FREE(image_quantized);

  FILE *ppm_file_out = fopen("remade.ppm", "wb");
  Image *back_to_rgb = to_image_rgb(image_unquantized);
  output_p6(ppm_file_out, back_to_rgb);

  fclose(ppm_file_out);
  IMG_FREE(image_unquantized);
  IMG_FREE(back_to_rgb);
}

int main(int argc, const char **argv) {
  assert(argc == 2, "Expect file name as argument.");
  // full_example(argv[1]);

  FILE *out = fopen("out.txt", "r");
  ImageYCbCr *saved = from_output_data(out);
  assert(saved->width > 0 && saved->height > 0, "Image dimensions invalid");
  fclose(out);

  ImageYCbCr *image_unquantized = idct(saved);
  IMG_FREE(saved);

  FILE *ppm_file_out = fopen("remade.ppm", "wb");
  Image *back_to_rgb = to_image_rgb(image_unquantized);
  IMG_FREE(image_unquantized);

  output_p6(ppm_file_out, back_to_rgb);
  fclose(ppm_file_out);

  IMG_FREE(back_to_rgb);
	return 0;
}



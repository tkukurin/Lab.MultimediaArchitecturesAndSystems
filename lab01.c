#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "common.c"

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

Image *ppm_p6_data(FILE *file, Header header) {
  int total = header.width * header.height;
  Image *image = malloc(sizeof(Image));
  image->width = header.width;
  image->height = header.height;
  image->data = malloc(total * sizeof(Rgb));
  size_t nread = fread(image->data, sizeof(Rgb), total, file);
  if (nread != total) {
    dbg("Total nread does not equal expected size.");
    if (image->data) {
      free(image->data);
    }

    free(image);
    return NULL;
  }
  return image;
}

YCbCr toYCbCr(Rgb rgb) {
  int R = rgb.r, G = rgb.g, B = rgb.b;
  int y  = (int)( 0.299   * R + 0.587   * G + 0.114   * B);
  int cb = (int)(-0.16874 * R - 0.33126 * G + 0.50000 * B);
  int cr = (int)( 0.50000 * R - 0.41869 * G - 0.08131 * B);
  return (YCbCr) { .y = y, .cb = cb, .cr = cr };
}

ImageYCbCr *toImageYCbCr(Image *image) {
  ImageYCbCr *ycc = malloc(sizeof(ImageYCbCr));
  ycc->width = image->width;
  ycc->height = image->height;
  int total = image->width * image->height;
  ycc->data = malloc(total * sizeof(YCbCr));
  for (int i = 0; i < total; i++)
    ycc->data[i] = toYCbCr(image->data[i]);
  return ycc;
}

#define DCT_BLOCK 8
ImageYCbCr *dct(ImageYCbCr *image) {
  ImageYCbCr *result = malloc(sizeof(ImageYCbCr));
  result->data = malloc(image->width * image->height * sizeof(YCbCr));

  double kc[2] = { 1.0 / sqrt(2), 1.0 };
  int width = image->width;
  int nxblocks = image->width / DCT_BLOCK;
  int nyblocks = image->height / DCT_BLOCK;
  int nblocks = nxblocks * nyblocks;

  for (int block = 0; block < nblocks; block++) {
    int off_y = width * DCT_BLOCK * (block / nxblocks);
    double y[DCT_BLOCK][DCT_BLOCK];
    double cb[DCT_BLOCK][DCT_BLOCK];
    double cr[DCT_BLOCK][DCT_BLOCK];

    for (int off_x = 0, i = 0; i < DCT_BLOCK; i++, off_x += width) {
      int starty = (block % nxblocks) * DCT_BLOCK + off_y + off_x;
      int endy = starty + DCT_BLOCK;
      for (int j = starty, k = 0; j < endy; j++, k++) {
        y[i][k] = image->data[j].y - 128;
        cb[i][k] = image->data[j].cb - 128;
        cr[i][k] = image->data[j].cr - 128;
      }
    }

    double dct_y[DCT_BLOCK][DCT_BLOCK];
    double dct_cb[DCT_BLOCK][DCT_BLOCK];
    double dct_cr[DCT_BLOCK][DCT_BLOCK];
    for (int i = 0; i < DCT_BLOCK; i++) {
      for (int j = 0; j < DCT_BLOCK; j++) {
        double cu = kc[i != 0];
        double cv = kc[j != 0];
        double ty = 0;
        double tcb = 0;
        double tcr = 0;
        for (int k = 0; k < DCT_BLOCK; k++) {
          for (int l = 0; l < DCT_BLOCK; l++) {
            double cos_val = cos((2 * k + 1) * i * M_PI / 16.0) 
              * cos((2 * l + 1) * j * M_PI / 16.0); 
            ty += y[k][l] * cos_val;
            tcb += cb[k][l] * cos_val;
            tcr += cr[k][l] * cos_val;
          }
        }

        dct_y[i][j] = 0.25 * cu * cv * ty;
        dct_cb[i][j] = 0.25 * cu * cv * tcb;
        dct_cr[i][j] = 0.25 * cu * cv * tcr;
      }
    }

    int quant_y[DCT_BLOCK][DCT_BLOCK];
    int quant_cb[DCT_BLOCK][DCT_BLOCK];
    int quant_cr[DCT_BLOCK][DCT_BLOCK];
    for (int i = 0; i < DCT_BLOCK; i++) {
      for (int j = 0; j < DCT_BLOCK; j++) {
        quant_y[i][j]  = round(dct_y[i][j] / k1[i][j]);
        quant_cb[i][j] = round(dct_cb[i][j] / k2[i][j]);
        quant_cr[i][j] = round(dct_cr[i][j] / k2[i][j]);
      }
    }

    for (int off_x = 0, i = 0; i < DCT_BLOCK; i++, off_x += width) {
      int starty = (block % nxblocks) * DCT_BLOCK + off_y + off_x;
      int endy = starty + DCT_BLOCK;
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
    fprintf(file, "%f ", image->data[i].y);
  fprintf(file, "\n");

  for(int i = 0; i < n_elements; i++)
    fprintf(file, "%f ", image->data[i].cb);
  fprintf(file, "\n");

  for(int i = 0; i < n_elements; i++)
    fprintf(file, "%f ", image->data[i].cr);
}

int main(int argc, const char **argv) {
  assert(argc == 2, "Expect file name as argument.");

  const char *file_name = argv[1];
  FILE *ppm_file = fopen(file_name, "r");
  assert(ppm_file != NULL, "Error opening input file.");

  Header header = ppm_header(ppm_file);
  assert(strcmp(header.ppm_id, "P6") == 0, "Expect P6 PPM type.");
  assert(header.max_rgb == 255, "Expect max 255 RGB");

  Image *image = ppm_p6_data(ppm_file, header);
  assert(image != NULL, "Error reading image.");
  fclose(ppm_file);

  ImageYCbCr *image_y_cb_cr = toImageYCbCr(image);
  free(image->data);
  free(image);
  assert(image_y_cb_cr != NULL, "Error creating YCbCr image.");

  ImageYCbCr *image_quantized = dct(image_y_cb_cr);
  free(image_y_cb_cr->data);
  free(image_y_cb_cr);
  assert(image_quantized != NULL, "Error creating DCT image.");

  FILE *out_file = fopen("out.txt", "w");
  output(out_file, image_quantized);
  fclose(out_file);
  free(image_quantized->data);
  free(image_quantized);

	return 0;
}



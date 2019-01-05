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

int main(int argc, const char **argv) {
  _assert(argc == 2, "Expect block 0-based ordinal number as argument.");
  FILE *pgm_file = fopen(LENNA0_FNAME, "r");
  _assert(pgm_file != NULL, "Error opening input file.");

  Header header = pgm_header(pgm_file);
  _assert(strcmp(header.ppm_id, "P5") == 0, "Expect P5 type.");
  _assert(header.max_grayscale >= 0 && header.max_grayscale < 65536, 
      "Expect valid max grayscale value.");
  if (getc(pgm_file) == '\r') {
    getc(pgm_file);
  }

  //Image *image = from_ppm_p6(pgm_file, header);
  //_assert(image != NULL, "Error reading image.");
  
  fclose(pgm_file);

  return 0;
}



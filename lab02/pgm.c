
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
  _assert(image != NULL, "Error reading image.");
  fclose(pgm_file);

  fprintf(stderr, "[%s] Loaded image.\n", fname);
  return image;
}

#define DEBUG 1

void dbg(const char *msg) {
  if (DEBUG) {
    fprintf(stderr, "[DEBUG] %s\n", msg);
  }
}

void print_arr(Rgb *xs, int d1, int d2, char fmt) {
  for (int i = 0; i < d1 * d2; i++) {
    Rgb data = *(xs + i);
    if (fmt == 'a') {
      printf("%d %d %d\n", data.r, data.g, data.b);
    } else {
      printf("%d ", fmt == 'r' ? data.r : fmt == 'g' ? data.g : data.b);
      if (i % d1 == 0 && i > 0)
        printf("\n");
    }
  }
  printf("\n");
}

void print_y_arr(YCbCr *xs, int d1, int d2, char fmt) {
  for (int i = 0; i < d1 * d2; i++) {
    YCbCr data = *(xs + i);
    if (fmt == 'a') {
      printf("%d %d %d\n", data.y, data.cb, data.cr);
    } else {
      printf("%d ", fmt == 'y' ? data.y : fmt == 'b' ? data.cb : data.cr);
      if (i % d1 == 0 && i > 0)
        printf("\n");
    }
  }
  printf("\n");
}

#define BLOCK_SIZE 16

#define COMMENT '#'
#define IMG_FREE(i) \
  if((i)->data) \
    free((i)->data); \
  free(i);

typedef int bool;

static const double k1[][8] =
{
  {16, 11, 10, 16, 24,  40,  51,  61},
  {12, 12, 14, 19, 26,  58,  60,  55},
  {14, 13, 16, 24, 40,  57,  69,  56},
  {14, 17, 22, 29, 51,  87,  80,  62},
  {18, 22, 37, 56, 68,  109, 103, 77},
  {24, 35, 55, 64, 81,  104, 113, 92},
  {49, 64, 78, 87, 103, 121, 120, 101},
  {72, 92, 95, 98, 112, 100, 103, 99}
};

static const double k2[][8] =
{
  {17, 18, 24, 47, 99, 99, 99, 99},
  {18, 21, 26, 66, 99, 99, 99, 99},
  {24, 26, 56, 99, 99, 99, 99, 99},
  {47, 66, 99, 99, 99, 99, 99, 99},
  {99, 99, 99, 99, 99, 99, 99, 99},
  {99, 99, 99, 99, 99, 99, 99, 99},
  {99, 99, 99, 99, 99, 99, 99, 99},
  {99, 99, 99, 99, 99, 99, 99, 99}
};

typedef enum {
  S_INIT, S_WIDTH, S_HEIGHT, S_MAX_COLOR
} State;

#define MAXBUF 1024
typedef struct {
  char buf[MAXBUF];
  size_t len;
} String;

typedef struct {
  char ppm_id[3];
  int width;
  int height;
  int max_grayscale;
} Header;

typedef unsigned char Grayscale;
typedef struct {
  size_t width;
  size_t height;
  Grayscale *data;
} Image;

void _err(const char *msg) {
  fprintf(stderr, "[ERROR] %s\n", msg);
}

void _assert(int condition_true, const char *msg) {
  if (!condition_true) {
    _err(msg);
    exit(1);
  }
}

void skip_line(FILE *file) {
  fscanf(file, "%*[^\n]\n");
}


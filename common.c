
#define COMMENT '#'
#define DEBUG 1 

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
  int max_rgb;
} Header;

typedef struct {
  unsigned char r;
  unsigned char g;
  unsigned char b;
} Rgb;

typedef struct {
  size_t width;
  size_t height;
  Rgb *data;
} Image;

typedef struct {
  int y;
  int cb;
  int cr;
} YCbCr;

typedef struct {
  size_t width;
  size_t height;
  YCbCr *data;
} ImageYCbCr;

void dbg(const char *msg) {
  if (DEBUG) {
    fprintf(stderr, "[DEBUG] %s\n", msg);
  }
}

void err(const char *msg) {
  fprintf(stderr, "[ERROR] %s\n", msg);
}

void assert(int condition_true, const char *msg) {
  if (!condition_true) {
    err(msg);
    exit(1);
  }
}

void skip_line(FILE *file) {
  fscanf(file, "%*[^\n]\n", NULL);
}

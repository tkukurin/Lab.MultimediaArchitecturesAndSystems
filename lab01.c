#include <stdio.h>
#include <stdlib.h>
#define COMMENT '#'

void assert(int condition_true, const char *msg) {
  if (!condition_true) {
    fprintf(stderr, "[ERROR] %s\n", msg);
    exit(1);
  }
}

#define MAXBUF 4096
typedef struct {
  char buf[MAXBUF];
  size_t size;
} String;

// Give next line in file. If *string is NULL, allocate.
// Caller is expected to dealloc.
String *next_line(FILE *file, String *string) {
  if (string == NULL)
    string = malloc(sizeof(String));

  size_t len = 0;
  string->size = fscanf(file, "%s", string->buf);
  //getline(&(string->buffer), &len, file);
  return string;
}

// Skip PPM file comments and return next line content.
String *ppm_skip_comments(FILE *file, String *string) {
  string = next_line(file, string);
  while (string->size > 0 && string->buf[0] == COMMENT) {
    string = next_line(file, string);
  }
  return string;
}

String *dealloc(String *s) {
  //if (s->buf)
    //free(s->buf);
  free(s);
  return NULL;
}

int main(int argc, const char **argv) {
  assert(argc == 2, "Expect single argument");

  const char *file_name = argv[1];
  FILE *file = fopen(file_name, "r");

  String *header = ppm_skip_comments(file, NULL);
  printf("%s", header->buf);

  fclose(file);

  header = dealloc(header);
	return 0;
}



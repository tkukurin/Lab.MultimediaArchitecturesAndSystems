#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int main() {
  double dbl = 2.0;
  float flt = 2.0;
  int i = 2;

  printf("%d\n", (int)flt);
  printf("%d\n", (int)dbl);

  printf("%f\n", flt);
  printf("%f\n", dbl);

  printf("i=%f\n", i);
  return 0;
}

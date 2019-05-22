#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  int *a, *b;
  int p[3];
  p[0] = 0;
  p[1] = atoi(argv[1]);
  p[2] = atoi(argv[2]);
  a = p + 1;
  b = a - 1; // a + 1
  if (*a >= *b) { // >=
    printf("%d\n", 0);
  } else {
    printf("%d\n", 1);
  }
  return 0;
}

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  int a, b, c, d;
  a = atoi(argv[1]);
  b = atoi(argv[2]);
  c = atoi(argv[3]);
  d = atoi(argv[4]);
  if (c == d) { // a > b
    printf("%d\n", 0);
  } else {
    printf("%d\n", 1);
  }
  return 0;
}

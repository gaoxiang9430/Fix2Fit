#include <stdio.h>
#include <stdlib.h>

struct foo {
  int dummy;
};

struct bar {
  struct foo f;
};


int main(int argc, char *argv[]) {
  int x, y;
  x = atoi(argv[1]);
  y = atoi(argv[2]);
  struct foo f;
  struct bar b;
  f = b.f; // this should be ignored   
  if (x > y) { // >=
    printf("%d\n", 0);
  } else {
    printf("%d\n", 1);
  }
  return 0;
}

#include <stdio.h>
#include <stdlib.h>

struct foo {
  int field;
};

int main(int argc, char *argv[]) {
  int a, b;
  a = atoi(argv[1]);
  b = atoi(argv[2]);
  struct foo *p = NULL;
  int dummy;
  dummy = p && p->field;
  if (a > b) { // >=
    printf("%d\n", 0);
  } else {
    printf("%d\n", 1);
  }
  return 0;
}

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  int a, b, c;
  int *p = NULL, *q = NULL, *t = NULL;
  a = atoi(argv[1]);
  switch (a) {
  case 0:
    p = &a;
    q = &b;
    t = &c;
    break;
  case 1:
    p = &a;
    q = &a;
    t = &c;
    break;
  case 2:
    p = &a;
    q = &b;
    t = &a;
    break;
  }
  if (p == q) { // p == t
    printf("%d\n", 0);
  } else {
    printf("%d\n", 1);
  }
  return 0;
}

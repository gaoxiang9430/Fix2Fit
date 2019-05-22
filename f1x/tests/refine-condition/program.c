#include <stdio.h>
#include <stdlib.h>

int foo() {
  return 0;
}

int main(int argc, char *argv[]) {
  int a, b;
  a = atoi(argv[1]);
  b = atoi(argv[2]);
  if (foo()) { // foo() || a >= b
    printf("%d\n", 0);
  } else {
    printf("%d\n", 1);
  }
  return 0;
}

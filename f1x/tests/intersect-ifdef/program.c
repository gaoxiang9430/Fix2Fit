#include <stdio.h>
#include <stdlib.h>

#define FOO

int foo() {
  return 1;
}

int main(int argc, char *argv[]) {
  int a, b;
  a = atoi(argv[1]);
  b = atoi(argv[2]);
  if (1 ||
#ifdef FOO
      1
#endif
      || foo()) {}
  if (a > b) { // >=
    printf("%d\n", 0);
  } else {
    printf("%d\n", 1);
  }
  return 0;
}

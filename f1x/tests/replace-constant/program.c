#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  int a, b;
  a = atoi(argv[1]);
  if (a > 1) { // a > 5
    printf("%d\n", 0);
  } else {
    printf("%d\n", 1);
  }
  return 0;
}

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  int n;
  n = atoi(argv[1]);
  while (n > 1) { // >=
    n--;
    printf("%d\n", n);
  }
  return 0;
}

#include <stdio.h>
#include <stdlib.h>

#include "file1.h"

int main(int argc, char *argv[]) {
  int n;
  n = atoi(argv[1]);
  f1(&n);
  printf("%d\n", n);
  return 0;
}

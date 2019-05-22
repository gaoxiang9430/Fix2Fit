#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main(int argc, char *argv[]) {
  int add, add2, mid, base;
  mid = 2081021665;
  base = -130689706;
  add = mid - base - 1; // mid - (unsigned)base - 1;
  printf("%d", add);
  return 0;
}

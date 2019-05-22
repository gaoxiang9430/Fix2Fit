#include <stdio.h>
#include <stdlib.h>

void inc(int* i) {
  (*i)++;
}

int main(int argc, char *argv[]) {
  int n;
  n = atoi(argv[1]);
  inc(&n); // delete
  printf("%d\n", n);
  return 0;
}

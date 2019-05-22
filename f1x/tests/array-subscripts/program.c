#include <stdio.h>

int main(int argc, char *argv[]) {
  int a[1];
  int b[1];
  a[0] = atoi(argv[1]);
  b[0] = atoi(argv[2]);
  if (a[0] > b[0]) { // >=
    printf("%d\n", 0);
  } else {
    printf("%d\n", 1);
  }
  return 0;
}

#include <stdio.h>
#include <stdlib.h>

#define FOO

int main(int argc, char *argv[]) {
  int a, b;
  a = atoi(argv[1]);
  b = atoi(argv[2]);
#ifdef FOO 
  if (a > b) { // >=
#endif
    printf("%d\n", 0);
  } else {
    printf("%d\n", 1);
  }
  return 0;
}

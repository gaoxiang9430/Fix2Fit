#include <stdio.h>
#include <stdlib.h>

struct Pair
{
  int first;
  int second;
};

int main(int argc, char *argv[]) {
  struct Pair p;
  struct Pair *pp = &p;
  pp->first = atoi(argv[1]);
  pp->second = atoi(argv[2]);
  if (pp->first > pp->second) { // >=
    printf("%d\n", 0);
  } else {
    printf("%d\n", 1);
  }
  return 0;
}

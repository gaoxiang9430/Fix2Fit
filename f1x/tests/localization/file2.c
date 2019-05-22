#include "file2.h"
#include "file3.h"
#include "lib.h"

void f2(int *i) {
  skip();
  skip();
  inc(i);
  skip();
  skip();
  skip();
  inc(i);
  skip();
  f3(i);
}

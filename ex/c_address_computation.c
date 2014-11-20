#include <assert.h>
#include <stdlib.h>

typedef double data;

int main() {
  int x;
  data *p1 = calloc(10, sizeof(data)), *p2 = p1+10;
  x= p2-p1;
  assert(x==10);
}

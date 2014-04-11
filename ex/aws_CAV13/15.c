#include "../../pagai_assert.h"
extern int unknown1();
extern int unknown2();
extern int unknown3();
extern int unknown4();

void main(int argc, char* argv[]) {

  int n;
  int i, k, j;


  n = unknown1();
  if(n < 1)
    return;
  if(k < n)
    return;
  j = 0;
  while( j <= n-1 ) {
    j++;
    k--;
  } 
  if(j < n)
    return;
  assert(k > -1);
}

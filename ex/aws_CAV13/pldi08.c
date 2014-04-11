#include "../../pagai_assert.h"
int main (){

  int x = -50;
  int y;
  while (x < 0){
     x = x + y;
     y++;
  }

  if (y < 0)
    goto ERROR;

  return 1;

ERROR: assert(0);


}


extern int input();
void rate_limiter() {
  int x_old;
  x_old = 0;
  while (1) {
    int x = input();
    if (x <= -1000 || x >= 1000) x=0;
    if (x >= x_old+1)
        x = x_old+1;
    if (x <= x_old-1)
        x = x_old-1;
    x_old = x;
  }
}


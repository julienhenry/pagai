extern int input();
void rate_limiter() {
  int x_old = 0;
  while (1) {
    int x = input();
    if (x < -100000 || x > 100000) x=0;
    if (x >= x_old+10) x = x_old+10;
    if (x <= x_old-10) x = x_old-10;
    x_old = x;
  }
}

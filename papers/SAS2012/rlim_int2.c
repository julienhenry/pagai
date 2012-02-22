void rate_limiter() {
  int x_old = 0;
  while (1) {
    int x = input(-100000, 100000);
    if (x > x_old+10) x = x_old+10;
    else if (x < x_old-10) x = x_old-10;
    else x_old = x;
  }
}

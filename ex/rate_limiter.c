
extern int input();
extern void assume(int predicate);

void rate_limiter() {
  int x_old;
  x_old = 0;
  while (1) {
    int x = input();
	assume (x >= -100000);
	assume (x <= 100000);
    if (x > x_old+10)
        x = x_old+10;
    if (x < x_old-10)
        x = x_old-10;
    x_old = x;
  }
}



int sum(unsigned n, int tab[n]) {
  int s = 0;
  for(unsigned i=0; i<n; i++) {
    s += tab[i];
  }
  return s;
}


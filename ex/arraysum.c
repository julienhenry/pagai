
double sum(unsigned n, double tab[n]) {
  double s = 0.0;
  for(unsigned i=0; i<n; i++) {
    s += tab[i];
  }
  return s;
}


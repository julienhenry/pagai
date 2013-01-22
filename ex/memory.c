
int global;

int f() {
	int tab[5];

	global = 1;
	for (int i = 0; i < 5; i++) {
		tab[i] = tab[i] + global;
	}
	global = global+ 5;
	return 0;

}

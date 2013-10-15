int f() {
	int i = 0;
	int j = 0;

	while (i < 50) {
		j = 0;
		while (j < 50) {
			i++;
			j++;
		}
		i = i-j+1;
	}
	return i+j;
}

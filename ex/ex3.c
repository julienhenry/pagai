
int f(int x) {
	return x+2;
}

int main(int argc, char ** argv) {
	int x;
	int y;
	int i;
	
	while (x) {
	x = 2;
	y = x + 50;
	y = y + 6;
	y = x + y + 5;

	for (i=0; i < y; i++) {
		x = x + 2;
	}

	}
	return x;
}

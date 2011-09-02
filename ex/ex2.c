
int f(int x) {
	return x+2;
}

int main(int argc, char ** argv) {
	int x;
	int y;
	int i;
	int toto;
	
	while (argc > 100000 || x >= 42) {
	x = 2;
	y = x + 50;
	y = y + 6;
	y = x + y + 5;

	//toto = x == y || x == 24;
	//if (toto) {
	//	x++;
	//}


	for (i=0; i < y; i++) {
//		x = x + 2;
//		x = f(x);
//		x = x - 2;
	}

	//x = x+1;

//	for (i=0; i < y; i++) {
//		x = x + 2;
//	}

	}
	return x;
}

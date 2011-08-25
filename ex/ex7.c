

int main(int argc, char ** argv) {

	int a,b,c,d,e,f,g,h;
	int x,y,z;
	int i, j;
	
	x = 0;
	y = 1000;
	a = 100;
	b = 200;
	d = 5;

	while (x < y) {
	
		for (i = 0; i < a; i++) {
			for (j = 0; j < b; j++) {
				if (j+i < 150) {
					z = 1;
					d += 3;
				} else {
					z = -1;
					d--;
				}
				x = x + z;
				y = y - z;
			}
		}
		
	}

	for (i = 0; i > c; i++) {
		c = c + d;
		while (y > 0) {
			y--;
			e = e+x+y;
			if (e < 0) {
				x--;
				d--;
			} else {
				x++;
				y++;
			}
		}
	}

	return 1;
}

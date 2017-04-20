#include <stdio.h>
#include <unistd.h>

int main(){

	int a, b, r, x, y, z;
	r = read(STDIN_FILENO, &a, 4);
	r = read(STDIN_FILENO, &b, 4);


    x = b*3;
    y = x-a;
    z = x+y;

	//printf("a = %d\n", a);
	//printf("b = %d\n", b);
	//printf("z = %d\n", z);

	return 0;
}

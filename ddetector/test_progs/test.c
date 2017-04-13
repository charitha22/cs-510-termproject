#include <stdio.h>

int main(){
	
	int a, b, x, y, z;
    fscanf(stdin, "%d", &a);
    fscanf(stdin, "%d", &b);

    x = b*3;
    y = x-a;
    z = x+y;

	return 0;
}

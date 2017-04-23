#include <stdio.h>
#include <unistd.h>

int main(){
	
	//int a, b, x, y, z;
    //fscanf(stdin, "%d", &a);
    //fscanf(stdin, "%d", &b);
	
	//printf("&a = %x\n", (int)&a);
	//printf("&b = %x\n", (int)&b);

	char a, b, r, x, y, z;
	r = read(STDIN_FILENO, &a, 1);
	r = read(STDIN_FILENO, &b, 1);


    x = b*3;
    y = x-a;
    z = x+y;

	return 0;
}

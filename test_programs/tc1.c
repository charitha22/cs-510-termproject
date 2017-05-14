#include <unistd.h>
//#include <stdio.h>
int main()
{
		char a,b,x; //b, x, y, z;
		
		read(0, &a, 1);
		read(0, &b, 1);
		
		//int k = (void*)&a;
		//printf("hello");
		
		x = a+b;
		//x = b * 3;
		//y = x - a;
		//z = x + y;

		return 0;
}

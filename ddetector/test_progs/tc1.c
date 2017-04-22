#include <unistd.h>
int main()
{
		char a, b, x, y, z;
		
		read(STDIN_FILENO, &a, 1);
		read(STDIN_FILENO, &b, 1);

		x = b * 3;
		y = x - a;
		z = x + y;

		return 0;
}

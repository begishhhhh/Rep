#include <stdio.h>

int main()
{
	printf("Enter two numbers\n");
	int a, b;
	scanf_s("%d %d", &a, &b);
	c = a + b;
	d = a * b;
	printf("%d %d\n", c, d);
	return 0;
}
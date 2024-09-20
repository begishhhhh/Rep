#include <stdio.h>

int main()
{
	printf("Enter two numbers\n");
	int a, b;
	scanf_s("%d %d", &a, &b);
	c = a + b;
	d = a * b;
	printf("%d\n", c);
	printf("%d\n", d);
	return 0;
}
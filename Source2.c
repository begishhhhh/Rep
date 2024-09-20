#include <stdio.h>

int main()
{
	printf("Enter two numbers\n");
	int a, b;
	scanf_s("%d %d", &a, &b);
	printf("%d", a + b);
	return 0;
}
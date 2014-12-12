#include <stdio.h>

extern int foo(int,int);

int main()
{
	int a, b;
	scanf("%d %d",&a,&b);
	printf("%d\n",foo(a,b));
}
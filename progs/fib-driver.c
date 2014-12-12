#include <stdio.h>

extern int fib(int);

int main()
{
    int n;
    scanf("%d",&n);

    printf("%d\n",fib(n));
}

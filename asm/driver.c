#include <stdio.h>

/* provide declarations of functions */
extern int foo(int,int);
extern int bar(int,int);

int main()
{
    int i, j;

    scanf("%d %d",&i,&j);

    printf("%d\n",foo(i,j));
    printf("%d\n",bar(i,j));
}

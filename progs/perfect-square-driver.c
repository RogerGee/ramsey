#include <stdio.h>

extern int hasSquare(int);

int main()
{
    int n;
    scanf("%d",&n);

    if (hasSquare(n))
        printf("yes\n");
    else
        printf("no\n");
}

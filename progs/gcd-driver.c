#include <stdio.h>

extern int gcd(int,int);
extern int lcm(int,int);

int main()
{
    int a, b;
    scanf("%d %d",&a,&b);
    printf("%d\n",gcd(a,b));
    printf("%d\n",lcm(a,b));
}

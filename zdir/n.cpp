#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <queue>
#include <unistd.h>

using namespace std;
int main()
{
    setuid(654);
    setgid(654);
    queue<int>q;
    q.push(1);
    q.pop();
    int x[123]={0};
    sort(x,x+120);
    int p;
    cin>>p;
    cout<<p;
    system("ls");
    int a=10;
    printf("%lf",sqrt(a));
}
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

int main()
{
    cout << "Hello World!" << endl;
    const char * hello = "hello";
    printf("%-10s %s\n", hello, hello);

    for (int i = 5; i < 15; i++)
    {
        printf("%-2d) %-40s*\n", i, "hell");
    }

    return 0;
}


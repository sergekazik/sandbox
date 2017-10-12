#include <iostream>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

int main()
{
    cout << "Hello World!" << endl;
    const char * hello = "hello";
    printf("%-10s %s\n", hello, hello);
    return 0;
}


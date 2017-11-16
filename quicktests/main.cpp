#include <iostream>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

static void forwarder(void);

class test
{
public:
    test();
    friend void forwarder(void);
private:
    void *something;

};
test::test()
{
    something = malloc(123);
    printf("something 0x%08x\n", something);
}
static test *pTest = new test();
void forwarder(void)
{
    if (pTest)
    {
        printf("forwarder something 0x%08x\n", pTest->something);
        free(pTest->something);
        pTest->something = NULL;
        delete pTest;
    }
}

int main()
{
    forwarder();
    return 0;

    cout << "Hello World!" << endl;
    const char * hello = "hello";
    printf("%-10s %s\n", hello, hello);

    for (int i = 5; i < 15; i++)
    {
        printf("%-2d) %-40s*\n", i, "hell");
    }

    return 0;
}


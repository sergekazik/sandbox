#include <iostream>
#include <stdio.h>
#include <stdlib.h>
using namespace std;

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#if 1
int main()
{
    const char *str = "abcdef";
    if (str[2] == 2[str])
        cout << strlen(str)+2 << " " << strlen(str+2) << endl;
    return 0;
}

#elif 0

void timer_handler (int signum)
{
    (void) signum;
    static int count = 0;
    printf ("timer expired %d times\n", ++count);
}

int start_timer (int ms)
{
    struct sigaction sa;
    struct itimerval timer;

    /* Install timer_handler as the signal handler for SIGVTALRM. */
    memset (&sa, 0, sizeof (sa));
    sa.sa_handler = &timer_handler;
    sigaction (SIGVTALRM, &sa, NULL);

    /* Configure the timer to expire after ms msec... */
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = ms*1000;
    /* ... and every ms msec after that. */ // - no repeat
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = ms*1000;;
    /* Start a virtual timer. It counts down whenever this process is executing. */
    setitimer (ITIMER_VIRTUAL, &timer, NULL);

    return 0;
}

int main()
{
    int count = 10;
    start_timer(500);
    printf("no exit from here\n");
    // getchar();
    while (1)
    {
    }
    return 0;
}

int main2()
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

#endif

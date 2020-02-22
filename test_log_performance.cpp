#include<stdio.h>
#include<string>
#include"Logging.h"
#include"TimerQueue.h"


extern int g_total;

void test()
{

    size_t start(detail::now());
    int n = 1000*10000;
    const bool kLongLog = false;
    string empty = " ";
    string longStr(3000, 'X');
    longStr += " ";
    for (int i = 0; i < n; ++i)
    {
        LOG<< "Hello 0123456789" << " abcdefghijklmnopqrstuvwxyz"
           << (kLongLog ? longStr : empty)
           << i;
    }
    size_t end(detail::now());
    size_t diff = end-start;
    double seconds = static_cast<double>(diff) / detail::kMicroSecondsPerSecond;

    printf("test: %f seconds, %d bytes, %10.2f msg/s, %.2f MiB/s\n",seconds, g_total, n / seconds, g_total / seconds / (1024 * 1024));

}

int main()
{
    test();

}

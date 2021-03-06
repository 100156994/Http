#pragma once

#include"Thread.h"
#include<string>
#include <unistd.h>

static int num=0;
MutexLock mutex;

void threadFunc()
{
    int k=10;
    while(k--)
    {
        {
            MutexLockGuard lock(mutex);
            ++num;
            printf("tid=%d num=%d\n", CurrentThread::tid(),num);
        }
	sleep(1);
    }

}


int main()
{
    Thread t1(threadFunc);
    Thread t2(threadFunc);
    t1.start();
    t2.start();

    t1.join();
    t2.join();
}

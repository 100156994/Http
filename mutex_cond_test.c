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
         MutexLockGuard lock(mutex);
        ++num;
        printf("tid=%d num=%d\n", CurrentThread::tid(),num);
    }

}


int main()
{
    Thread t1=Thread(threadFunc);
    Thread t2=Thread(threadFunc);
    t1.start();
    t2.start();

    t1.join();
    t2.join();
}

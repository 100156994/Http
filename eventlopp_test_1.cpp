#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include"EventLoop.h"
#include"Thread.h"


EventLoop* g_loop;

void callback()
{
  printf("callback(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
  EventLoop anotherLoop;
}

void callback1()
{
  printf("callback1(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
  printf("test runafter in another thread\n");
}

void threadFunc()
{
  printf("threadFunc(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());

  assert(EventLoop::getEventLoopOfCurrentThread() == NULL);
  EventLoop loop;
  assert(EventLoop::getEventLoopOfCurrentThread() == &loop);
  loop.runAfter(1.0, callback);
  gloop->runAfter(1.0,callback1);
  loop.loop();
}

int main()
{
  printf("main(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());

  assert(EventLoop::getEventLoopOfCurrentThread() == NULL);
  EventLoop loop;
  assert(EventLoop::getEventLoopOfCurrentThread() == &loop);

  Thread thread(threadFunc);
  thread.start();

  loop.loop();
}

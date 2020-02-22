## 一个简单的网络库和静态Http服务器

#介绍
项目初衷是看了《linux多线程服务端编程》，于是想仿照muduo写一个网络库和http服务器，来综合运用之前学习的知识，包括C++ TCP/IP协议 网络编程的IO模型 多线程编程 linux下测试 makefile构建等等内容。网络库的架构基本和muduo相同，精简了一些内容，异步日志也没有写的那么复杂，HTTP服务器解析了GET,HEAD请求，支持长连接和管线化。很多内容参考了/chenshuo/muduo和linyacool/WebServer。

#
Reactor模式，事件驱动，事件回调用funciton+bind
使用Epoll LT模式来做IO复用，非阻塞IO
线程模型为one loop per thread，主线程accept请求，其余IO线程用来处理连接，静态http服务器所以没有计算线程，另外还有一个日志线程
线程池来避免线程创建销毁的开销，线程的分配为轮询
timerfd作为定时器，实现定时器队列
eventfd实现线程异步唤醒，防止线程阻塞在epoll_wait上
线程间同步 使用锁，主线程和某一IO线程争用任务队列的一把锁
封装pthread线程库的一些内容，没有使用std::thread
std::automic 避免一些竞态
RAII避免内存泄漏
双缓冲的异步日志，没有设定日志等级 
queueInLoop 做跨线程调用

#连接
建立连接：事先打开一个null文件，fd上限后关闭null接收然后马上关闭在打开null。如果有其他线程也打开fd则又可能竞态，不过服务器只有一个线程打开fd
关闭连接：只能被动关闭，主动关闭写端(会在没有写时关闭)  对端正常关闭：read 0 就直接关闭   RST epollhup  真正的关闭在析构时 这时缓冲区已经没有数据了   需要对端保证主动关闭 


#测试：
环境：
ubuntu 18.04
g++ 4.8
笔记本 CPU i5-8400  8G内存

使用工具Webbench，开启1000客户端进程，时间为60s
分别测试短连接和长连接的情况
关闭所有的输出及Log，响应为"Hello World"和一定的HTTP头部
服务器线程池为4

仿照muduo的结构，测试结果基本相近
短连接QPS：96000左右  长连接QPS：300000左右

线程4,5达到性能最好，开更多导致线程间调度开销变大，开的少没有跑满CPU

长连接数量明显更多，没有建立和关闭的开销，属于正常情况。

开始测试的时候发现自己写的和muduo性能差距很大，排查问题的时候顺便测试了muduo使用poll的性能，以及用accept+fctnl和accept4的区别，accept4会有一定提升，可能因为系统调用更少，poll会比epoll性能高一点，不过影响很小，测试次数不多不是肯定。以上均在编译时O2优化的情况下测试的，测试有的时候忘记开优化真的辛酸泪。


#碎碎念
事件驱动+回调一开始感觉自己理解了看懂了，但真正去写的时候回调多了还是容易头晕，要对整个回调的逻辑整理清除。
TcpConnection的生命周期问题，本来只写了网络库的服务端，这样服务器是有连接的ptr的，服务器删除时会回调destroy，这样保证epoll的channel的remove 用户持有的话也只是销毁一个被remove的Tcp  因为服务器只有在conn的close回调用删除连接，而且destroy的回调会在loop的dopendingfenc中执行不会在handleEvent中执行，所以不用担心channel的生命期问题了。后来加入客户端的时候，发现客户析构时候TcpConnection的生命周期就出现了问题，因为客户和Loop不是一个线程，TcpConn析构的时候可能Loop里channel还在epoll中，后来细想问题也不大只是多了一些用不到的指针在epoll中 。client的回调也要注意,可能client析构了,但是channel还要回调client的成员。回调要很注意生命期问题，除非每个回调都用shared
测试的时候也遇到很多坑，webbench默认的1.1请求居然是connection:close,一开始没想这点还一直奇怪为什么strace时候长连接每次都会close。ab的请求虽然支持长连接，但是是1.0版本的，而服务器只解析1.1版本，结果一直在服务器连接上找问题，没想到http请求的原因。多线程调试的时候，发现会有回调没初始化，以及定时器传0等等。
测试日志的时候，默认用了大的buffer，结果写日志巨慢，然后gdb发现分配内存还有调用对齐，以及本身大内存分配比小内存慢。

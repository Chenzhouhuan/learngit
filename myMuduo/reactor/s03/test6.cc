#include "lib/EventLoop.h"
#include "lib/EventLoopThread.h"
#include <stdio.h>

void runInThread(){
    printf("runInThread(): pid = %d, tid = %d\n", getpid(), muduo::CurrentThread::tid()); //获取进程和线程的id
                                                                                          //Gets the id of the process and thread
}

int main(){
    printf("main(): pid = %d, tid = %d\n", getpid(), muduo::CurrentThread::tid());

    muduo::EventLoopThread loopThread;
    muduo::EventLoop* loop = loopThread.startLoop();
    loop->runInLoop(runInThread);
    sleep(1);
    loop->runAfter(2,runInThread);
    sleep(3);
    loop->quit();

    printf("exit main().\n");
}
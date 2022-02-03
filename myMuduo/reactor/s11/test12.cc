#include "lib/Connector.h"
#include "lib/EventLoop.h"
#include "lib/EventLoopThreadPool.h"

#include <stdio.h>
#include <math.h>
#include <vector>
#include <boost/bind.hpp>

muduo::EventLoop* g_loop;

void connectCallback(int sockfd){
    printf("connected.\n");
    // g_loop->quit();
}

int main(int argc, char* argv[]){
    muduo::EventLoop loop;
    g_loop = &loop;
    muduo::InetAddress addr("127.0.0.1", 9981);
    if(argc > 1){
        muduo::EventLoopThreadPool pool(g_loop);
        std::vector<muduo::ConnectorPtr> connectList(atoi(argv[1]));
        pool.setThreadNum(ceil(atoi(argv[1])/3));
        pool.start();
        for(int i = 0; i < atoi(argv[1]); ++i){
            muduo::EventLoop* ioloop = pool.getNextLoop();
            muduo::ConnectorPtr connector(new muduo::Connector(ioloop, addr));
            connectList[i] = connector;
            connector->setNewConnectionCallback(connectCallback);
            ioloop->runInLoop(boost::bind(&muduo::Connector::start, connector));
        }
        loop.loop();
    }else{
        muduo::ConnectorPtr connector(new muduo::Connector(&loop, addr));
        connector->setNewConnectionCallback(connectCallback);
        connector->start();
        loop.loop();
    }
}
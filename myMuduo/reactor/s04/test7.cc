#include "lib/Acceptor.h"
#include "lib/EventLoop.h"
#include "lib/InetAddress.h"
#include "lib/SocketsOps.h"
#include <stdio.h>
#include <time.h>

void newConnection(int sockfd, const muduo::InetAddress& peerAddr){
    printf("newConnection(): accepted a new connection from %s\n", peerAddr.toHostPort().c_str());
    // printf("start write...\n");
    char str[20];
    time_t tt = time(NULL);//这句返回的只是一个时间cuo
    tm* t= localtime(&tt);
    snprintf(str, 1024, "now: %d-%02d-%02d %02d:%02d:%02d\n", 
            t->tm_year + 1900,
            t->tm_mon + 1,
            t->tm_mday,
            t->tm_hour,
            t->tm_min,
            t->tm_sec);
    // ::write(sockfd, "How are you?\n", 13);
    ::write(sockfd, str, strlen(str));
    // printf("finish write.\n");
    muduo::sockets::close(sockfd);
}

void newConnection_2(int sockfd, const muduo::InetAddress& peerAddr){
    printf("newConnection_2(): accepted a new connection from %s\n", peerAddr.toHostPort().c_str());
    char str[25];
    snprintf(str, 1024, "This is the second port\n");
    ::write(sockfd, str, strlen(str));
    muduo::sockets::close(sockfd);
}

int main(){
    printf("main(): pid = %d\n", getpid());

    muduo::InetAddress listenAddr(9981); //9981 is worng
    muduo::InetAddress listenAddr_2(10086); //another port;
    muduo::EventLoop loop;

    muduo::Acceptor acceptor(&loop, listenAddr);
    muduo::Acceptor acceptor_2(&loop, listenAddr_2);
    acceptor.setNewConnectionCallback(newConnection);
    acceptor.listen();

    acceptor_2.setNewConnectionCallback(newConnection_2);
    acceptor_2.listen();

    loop.loop();
}
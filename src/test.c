/*
 * test.c
 *
 *  Created on: Apr 29, 2017
 *      Author: magic
 */


#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>


#define MAXLINE 5
#define OPEN_MAX 100
#define LISTENQ 20
#define SERV_PORT 5000
#define INFTIM 1000

void setnonblocking(int sock)
{
    int opts;
    opts=fcntl(sock,F_GETFL);
    if(opts<0)
    {
        perror("fcntl(sock,GETFL)");
        exit(1);
    }
    opts = opts|O_NONBLOCK;
    if(fcntl(sock,F_SETFL,opts)<0)
    {
        perror("fcntl(sock,SETFL,opts)");
        exit(1);
    }
}

int main(int argc, char* argv[])
{
    int i, maxi, listenfd, connfd, sockfd,epfd,nfds, portnumber;
    ssize_t n;
    char line[MAXLINE];
    socklen_t clilen;

    struct epoll_event ev,events[20];
    //生成用于处理accept的epoll专用的文件描述符

    epfd=epoll_create(256);
    struct sockaddr_in clientaddr;
    struct sockaddr_in serveraddr;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    ev.data.fd = listenfd;
    ev.events = EPOLLIN | EPOLLET;

    //注册epoll事件
    epoll_ctl(epfd,EPOLL_CTL_ADD,listenfd,&ev);
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    char *local_addr="127.0.0.1";
    inet_aton(local_addr,&(serveraddr.sin_addr));//htons(portnumber);

    serveraddr.sin_port=htons(portnumber);
    bind(listenfd,(sockaddr *)&serveraddr, sizeof(serveraddr));
    listen(listenfd, LISTENQ);
    maxi = 0;
    for ( ; ; ) {
        //等待epoll事件的发生

        nfds=epoll_wait(epfd,events,20,500);
        //处理所发生的所有事件

        for(i=0;i<nfds;++i)
        {
            if(events[i].data.fd==listenfd)//如果新监测到一个SOCKET用户连接到了绑定的SOCKET端口，建立新的连接。
            {
                connfd = accept(listenfd,(sockaddr *)&clientaddr, &clilen);
                if(connfd<0){
                    perror("connfd<0");
                    exit(1);
                }

                char *str = inet_ntoa(clientaddr.sin_addr);
                printf( "accapt a connection from %s\n", str);
                //设置用于读操作的文件描述符

                ev.data.fd=connfd;
                //设置用于注测的读操作事件

                ev.events=EPOLLIN|EPOLLET;
                //ev.events=EPOLLIN;

                //注册ev

                epoll_ctl(epfd,EPOLL_CTL_ADD,connfd,&ev);
            }
            else if(events[i].events&EPOLLIN)//如果是已经连接的用户，并且收到数据，那么进行读入。

            {
                printf( "EPOLLIN \n" );
                if ( (sockfd = events[i].data.fd) < 0)
                    continue;
                if ( (n = read(sockfd, line, MAXLINE)) < 0) {
                    if (errno == ECONNRESET) {
                        close(sockfd);
                        events[i].data.fd = -1;
                    } else
                        printf("readline error\n");
                } else if (n == 0) {
                    close(sockfd);
                    events[i].data.fd = -1;
                }
                line[n] = '/0';
                printf("read %s", line);
                //设置用于写操作的文件描述符

                ev.data.fd=sockfd;
                //设置用于注测的写操作事件

                ev.events=EPOLLOUT|EPOLLET;
                //修改sockfd上要处理的事件为EPOLLOUT

                //epoll_ctl(epfd,EPOLL_CTL_MOD,sockfd,&ev);

            }
            else if(events[i].events&EPOLLOUT) // 如果有数据发送

            {
                sockfd = events[i].data.fd;
                write(sockfd, line, n);
                //设置用于读操作的文件描述符

                ev.data.fd=sockfd;
                //设置用于注测的读操作事件

                ev.events=EPOLLIN|EPOLLET;
                //修改sockfd上要处理的事件为EPOLIN

                epoll_ctl(epfd,EPOLL_CTL_MOD,sockfd,&ev);
            }
        }
    }
    return 0;
}
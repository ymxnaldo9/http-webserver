#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <signal.h>
#include "locker.h"
#include "threadpool.h"
#include "http_conn.h"
#include "wrap.h"
#include "utils.h"

#define MAX_FD 65535
#define MAX_EVENT 10000

void addsig(int sig, void(handler)(int)) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    sigfillset(&sa.sa_mask);
    sigaction(sig, &sa, NULL);
}

int main(int argc, char* argv[]) {
    if (argc <= 1) {
        printf("please input port number\n");
        exit(0);
    }
    int port = atoi(argv[1]);
    epoll_event events[MAX_EVENT];
    addsig(SIGPIPE, SIG_IGN);
    //build threadpool and initialize
    threadpool<http_conn>* pool = NULL;
    try {
        pool = new threadpool<http_conn>;
    }
    catch(...) {
        exit(-1);
    }
    http_conn* users = new http_conn[MAX_FD];
    int lfd = Socket(AF_INET, SOCK_STREAM, 0);
    int reuse = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    Bind(lfd, (struct sockaddr*)&address, sizeof(address));
    Listen(lfd, 128);
    int epfd = epoll_create(MAX_EVENT);
    addfd(epfd, lfd, false);
    http_conn::m_epfd = epfd;
    while (1) {
        int ret = epoll_wait(epfd, events, MAX_EVENT, -1);
        if (ret < 0 && errno != EINTR) {
            printf("epoll failed\n");
            break;
        }
        for (int i = 0; i < ret; i++) {
            int sfd = events[i].data.fd;
            if (sfd == lfd) {
                struct sockaddr_in clit_addr;
                socklen_t clit_addr_len = sizeof(clit_addr);
                int cfd = Accept(lfd, (struct sockaddr*)&clit_addr, &clit_addr_len);
                if (http_conn::m_user_count >= MAX_FD) {
                    close(cfd);
                    continue;
                }
                users[cfd].init(cfd, clit_addr);
            }
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                users[sfd].close_conn();
            }
            else if (events[i].events & EPOLLIN) {
                if (users[sfd].read()) {
                    pool->append(users + sfd);
                }
                else {
                    users[sfd].close_conn();
                }
            }
            else if (events[i].events & EPOLLOUT) {
                if (!users[sfd].write()) {
                    users[sfd].close_conn();
                }
            }
        }
    }
    close(epfd);
    close(lfd);
    delete[] users;
    delete[] pool;
    return 0;
}
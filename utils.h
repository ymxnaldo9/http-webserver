#ifndef UTILS_H
#define UTILS_H

#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

void setnonblocking(int fd);
void addfd(int epfd, int fd, bool one_shot);
void modfd(int epfd, int fd, int ev);
void removefd(int epfd, int fd);

#endif
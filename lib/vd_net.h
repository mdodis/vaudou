
// vd_net.h - A C networking library
// 
// -------------------------------------------------------------------------------------------------
// MIT License
// 
// Copyright (c) 2024 Michael Dodis
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#if defined(__INTELLISENSE__) || defined(__CLANGD__)
#define VD_NET_IMPLEMENTATION
#endif

#ifndef VD_NET_H
#define VD_NET_H

enum {
    VD_NET_FTP_DEFAULT_PORT = 21,
};

typedef struct {
   int port;
} VD_NetFtp;

int vd_net_ftp(VD_NetFtp *conf);

#ifdef VD_NET_IMPLEMENTATION

void vd_net__itoa(int number, char *buffer, int buffer_size)
{
    int count = 0; // characters written
    do
    {
        int div = number / 10;
        int mod = number % 10;

        memmove(buffer + 1, buffer, count);
        buffer[0] = '0' + mod;

        number = div;
        count++;
    } while (number != 0 && count < buffer_size);

    buffer[count] = 0;
}

int vd_net_ftp(VD_NetFtp *conf)
{
    char port_str[33];
    int retcode;
    int sockfd;
    int yes = 1;
    vd_net__itoa(conf->port, port_str, sizeof(port_str));
    printf("NUMBER %s\n", port_str);
    struct addrinfo hints = {
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
        .ai_flags = AI_PASSIVE,
    };

    struct addrinfo *servinfo;
    // getaddrinfo(0, port_str, &hints, &res);
    if ((retcode = getaddrinfo(0, port_str, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(retcode));
        return 1;
    }

    struct addrinfo *p;
    for (p = servinfo; p != 0; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int) == -1)) {
            exit(-1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo);

    if (p == 0) {
        fprintf(stderr, "Failed to bind socket\n");
        return -1;
    }
    return 0;
}

#endif // VD_NET_IMPLEMENTATION
#endif // VD_NET_H

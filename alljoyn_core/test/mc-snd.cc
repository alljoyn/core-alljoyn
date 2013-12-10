/******************************************************************************
 * Copyright (c) 2010 - 2011, AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 ******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define HELLO_PORT 9956
#define HELLO_GROUP "239.255.37.41"
#define HELLO_GROUP_IPV6 "ff03::239.255.37.41"

#define IPV4 1
#define IPV6 0

int main(int argc, char** argv)
{
    printf("%s main()\n", argv[0]);

    uint32_t ms = 999;
    uint32_t n = 1000000;
    bool verbose = false;
    char const* address = "0.0.0.0";

    for (int i = 1; i < argc; ++i) {
        if (0 == strcmp("-a", argv[i])) {
            address = argv[i + 1];
            i += 1;
        } else if (0 == strcmp("-m", argv[i])) {
            ms = atoi(argv[i + 1]);
            i += 1;
        } else if (0 == strcmp("-n", argv[i])) {
            n = atoi(argv[i + 1]);
            i += 1;
        } else if (0 == strcmp("-c", argv[i])) {
            verbose = true;
        } else {
            printf("Unknown option %s\n", argv[i]);
            exit(0);
        }
    }

    printf("verbose == %d\n", verbose);
    printf("ms == %d\n", ms);
    printf("n == %d\n", n);
    printf("address == %s\n", address);

#if IPV4
    int fd4;
#endif

#if IPV6
    int fd6;
#endif

#if IPV4
    if ((fd4 = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }
#endif

#if IPV6
    if ((fd6 = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
        perror("socket6");
        exit(1);
    }
#endif

#if IPV4 || IPV6
    uint32_t yes = 1;
#endif

#if IPV4
    if (setsockopt(fd4, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&yes), sizeof(yes)) < 0) {
        perror("setsockopt SO_REUSEADDR");
        exit(1);
    }
#endif

#if IPV6
    if (setsockopt(fd6, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&yes), sizeof(yes)) < 0) {
        perror("setsockopt6 SO_REUSEADDR");
        exit(1);
    }
#endif

#if IPV4 || IPV6
    uint32_t ttl = 1;
#endif

#if IPV4
    if (setsockopt(fd4, IPPROTO_IP, IP_MULTICAST_TTL, reinterpret_cast<const char*>(&ttl), sizeof(ttl)) < 0) {
        perror("setsockopt IP_MULTICAST_TTL");
        exit(1);
    }
#endif

#if IPV6
    if (setsockopt(fd6, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, reinterpret_cast<const char*>(&ttl), sizeof(ttl)) < 0) {
        perror("setsockopt IPV6_MULTICAST_HOPS");
        exit(1);
    }
#endif

#if IPV4
    struct in_addr addr;
    inet_aton(address, &addr);
    if (setsockopt(fd4, IPPROTO_IP, IP_MULTICAST_IF, &addr, sizeof(addr)) < 0) {
        perror("sendto");
        exit(1);
    }
#endif

#if IPV6
    uint32_t index = 0;
    socklen_t indexLen = sizeof(index);

    if (getsockopt(fd6, IPPROTO_IPV6, IPV6_MULTICAST_IF, &index, &indexLen) < 0) {
        perror("getsockopt IPV6_MULTICAST_IF");
        exit(1);
    }

    if (setsockopt(fd6, IPPROTO_IPV6, IPV6_MULTICAST_IF, &index, sizeof(index)) < 0) {
        perror("sendto");
        exit(1);
    }
#endif

#if IPV4
    struct sockaddr_in sockaddr;
    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(HELLO_PORT);
    sockaddr.sin_addr.s_addr = inet_addr(HELLO_GROUP);
#endif

#if IPV6
    struct sockaddr_in6 sockaddr6;
    memset(&sockaddr6, 0, sizeof(sockaddr6));
    sockaddr6.sin6_family = AF_INET6;
    sockaddr6.sin6_port = htons(HELLO_PORT);
    if (inet_pton(AF_INET6, HELLO_GROUP_IPV6, &sockaddr6.sin6_addr) == 0) {
        perror("inet_pton");
        exit(1);
    }
#endif

    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 1000 * 1000 * ms;

    char buf[1024];
    for (uint32_t i = 0; i < n; ++i) {
        snprintf(buf, 1024, "H%d", i);
#if IPV4
        if (sendto(fd4, buf, 1 + strlen(buf), 0, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0) {
            perror("sendto");
        }
#endif
#if IPV6
        if (sendto(fd6, buf, 1 + strlen(buf), 0, (struct sockaddr*)&sockaddr6, sizeof(sockaddr6)) < 0) {
            perror("sendto");
        }
#endif
        printf("%s\n", buf);
        nanosleep(&ts, NULL);
    }

    const char* endmessage = "G";
    for (uint32_t i = 0; i < 5; ++i) {
#if IPV4
        sendto(fd4, endmessage, strlen(endmessage), 0, (struct sockaddr*)&sockaddr, sizeof(sockaddr));
#endif
#if IPV6
        sendto(fd6, endmessage, strlen(endmessage), 0, (struct sockaddr*)&sockaddr6, sizeof(sockaddr6));
#endif
        printf("%s\n", endmessage);
        nanosleep(&ts, NULL);
    }

    return 0;
}


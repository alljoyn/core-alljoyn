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

#define MSGBUFSIZE 256

int main(int argc, char** argv)
{
    printf("%s main()\n", argv[0]);

#if IPV4 && IPV6
    printf("For now, either IPV4 or IPV6, not both.\n");
    exit(1);
#endif

#if IPV4
    char const* address = "0.0.0.0";
#endif

#if IPV6
    char const* address = "0:0:0:0:0:0:0:0";
#endif

    for (int i = 1; i < argc; ++i) {
        if (0 == strcmp("-a", argv[i])) {
            address = argv[i + 1];
            i += 1;
        } else {
            printf("Unknown option %s\n", argv[i]);
            exit(0);
        }
    }

    printf("address == %s\n", address);

#if IPV4
    int fd4;
#endif

#if IPV6
    int fd6;
#endif

#if IPV4 || IPV6
    int nbytes;
    char msgbuf[MSGBUFSIZE];
    uint32_t yes = 1;
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

#if IPV4
    if (setsockopt(fd4, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        perror("Reusing ADDR failed");
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
        perror("IP_MULTICAST_TTL");
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
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(HELLO_PORT);

    if (bind(fd4, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }
#endif

#if IPV6
    struct sockaddr_in6 addr;
    socklen_t addrlen = sizeof(addr);

    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_addr = in6addr_any;
    addr.sin6_port = htons(HELLO_PORT);

    if (bind(fd6, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }
#endif

#if IPV4
    struct ip_mreq mreq;

    mreq.imr_multiaddr.s_addr = inet_addr(HELLO_GROUP);
    inet_aton(address, (struct in_addr*)&mreq.imr_interface.s_addr);

    printf("mreq.imr_interface.s_addr == 0x%x\n", mreq.imr_interface.s_addr);

    if (setsockopt(fd4, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        perror("setsockopt IP_ADD_MEMBERSHIP");
        exit(1);
    }
#endif

#if IPV6

    struct ipv6_mreq mreq;

    if (inet_pton(AF_INET6, HELLO_GROUP_IPV6, &mreq.ipv6mr_multiaddr) == 0) {
        perror("inet_pton");
        exit(1);
    }

    uint32_t index = 0;
    socklen_t indexLen = sizeof(index);
    if (getsockopt(fd6, IPPROTO_IPV6, IPV6_MULTICAST_IF, &index, &indexLen) < 0) {
        perror("getsockopt IPV6_MULTICAST_IF");
        exit(1);
    }
    mreq.ipv6mr_interface = index;

    if (setsockopt(fd6, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        perror("setsockopt IPV6_ADD_MEMBERSHIP");
        exit(1);
    }
#endif

    uint32_t n = 0;
    while (1) {
#if IPV4
        if ((nbytes = recvfrom(fd4, msgbuf, MSGBUFSIZE, 0, (struct sockaddr*)&addr, &addrlen)) < 0) {
            perror("recvfrom");
            exit(1);
        }
#endif

#if IPV6
        if ((nbytes = recvfrom(fd6, msgbuf, MSGBUFSIZE, 0, (struct sockaddr*)&addr, &addrlen)) < 0) {
            perror("recvfrom");
            exit(1);
        }
#endif

#if IPV4 || IPV6
        if (msgbuf[0] == 'G') {
            break;
        }
        if (msgbuf[0] == 'H') {
            printf("%s - %d\n", msgbuf, nbytes);
            ++n;
        }
#endif
    }

    printf("n == %d\n", n);
    exit(0);
}

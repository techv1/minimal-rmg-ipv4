#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MCAST_BLOCK_SOURCE 43
#define MCAST_JOIN_GROUP   42

void test_mcast_block_source(void) {
    printf("[TEST] MCAST_BLOCK_SOURCE (optname 43) - benign test\n");

    int fd = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    if (fd < 0) {
        printf("[FAIL] socket() failed errno=%d\n", errno);
        return;
    }

    unsigned char gsr[264];
    memset(gsr, 0, sizeof(gsr));

    /* Build a minimal valid group_source_req */
    struct sockaddr_in *sin_grp = (struct sockaddr_in *)&gsr[8];
    sin_grp->sin_family = AF_INET;
    sin_grp->sin_addr.s_addr = inet_addr("239.255.0.1");

    struct sockaddr_in *sin_src = (struct sockaddr_in *)&gsr[0x88];
    sin_src->sin_family = AF_INET;
    sin_src->sin_addr.s_addr = inet_addr("192.0.2.1");

    errno = 0;
    long ret = setsockopt(fd, IPPROTO_IP, MCAST_BLOCK_SOURCE, gsr, sizeof(gsr));
    printf("[RESULT] setsockopt(MCAST_BLOCK_SOURCE) ret=%ld errno=%d (%s)\n",
           ret, errno, strerror(errno));

    close(fd);
}

void test_mcast_join_group(void) {
    printf("\n[TEST] MCAST_JOIN_GROUP (optname 42) - benign test\n");

    int fd = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    if (fd < 0) {
        printf("[FAIL] socket() failed errno=%d\n", errno);
        return;
    }

    unsigned char gr[136];
    memset(gr, 0, sizeof(gr));

    struct sockaddr_in *sin_grp = (struct sockaddr_in *)&gr[8];
    sin_grp->sin_family = AF_INET;
    sin_grp->sin_addr.s_addr = inet_addr("239.255.0.2");

    errno = 0;
    long ret = setsockopt(fd, IPPROTO_IP, MCAST_JOIN_GROUP, gr, sizeof(gr));
    printf("[RESULT] setsockopt(MCAST_JOIN_GROUP) ret=%ld errno=%d (%s)\n",
           ret, errno, strerror(errno));

    close(fd);
}

int main(void) {
    printf("========================================\n");
    printf("Benign MCAST Test Harness for SM-X216B\n");
    printf("========================================\n\n");

    test_mcast_join_group();
    test_mcast_block_source();

    printf("\n========================================\n");
    printf("Benign MCAST tests complete\n");
    printf("========================================\n");

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/epoll.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>

#include <assert.h>
#include <errno.h>

#include "net.h"
#include "log.h"


/* returns an ipstr struct that contains the presentation string converted ipaddress and port */
struct ipstr get_ip_str(struct sockaddr_storage *ss) {
    struct sockaddr_in *addr_in = (struct sockaddr_in *)get_in_addr((struct sockaddr *)ss);
    struct ipstr str = {0};
    
    const char *paddress = NULL;
    int status = -1;
    uint16_t port = 0;

    assert(ss != NULL);

    if (ss->ss_family == AF_INET) {
        struct sockaddr_in *addr_in = (struct sockaddr_in *)ss;
        paddress = inet_ntop(AF_INET, &(addr_in->sin_addr), str.address, sizeof(str.address));
        port = ntohs(addr_in->sin_port);
    } else if (ss->ss_family == AF_INET6) {
        struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)ss;
        paddress = inet_ntop(AF_INET6, &(addr_in6->sin6_addr), str.address, sizeof(str.address));
        port = ntohs(addr_in6->sin6_port);
    } else {
        logfmt(stderr, ERROR, "Unknown address family: %d\n", ss->ss_family);
        exit(EXIT_FAILURE);
    }

    if (paddress == NULL) {
        perror("inet_ntop");
        exit(EXIT_FAILURE);
    }

    status = snprintf(str.port, MAX_PORT_LEN, "%u", port);
    if (status < 0 || status >= MAX_PORT_LEN) {
        logfmt(stderr, ERROR, "snprintf error or truncation\n");
    }

    return str;
}

/* returns the sockaddr_in pointer of the specified sa_family */
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
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

#include "client.h"
#include "log.h"

/* returns the sockaddr_in pointer of the specified sa_family */
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

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

/* recv's into *out, returns recv'd nbytes on success, on failure/disconnect -1, on timeout -2 */
ssize_t read_socket(int sockfd, char *out, size_t out_size, int flags) {
    if (out == NULL || out_size == 0) {
        logfmt(stderr, ERROR, "read_socket: invalid buffer\n");
        return -1;
    }


    memset(out, 0, out_size);

    ssize_t nbytes = recv(sockfd, out, out_size - 1, flags);
    if (nbytes < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) return -2; // timeout
        if (errno == ECONNRESET) return -1; // disconnect
        perror("recv");
        return -1;
    }

    // cut the string off (we read in out_size - 1)
    out[nbytes] = '\0';

    logfmt(stdout, ERROR, "received %ld bytes on socket %d\n", nbytes, sockfd);
    return nbytes;
}



/* copies string literal *in and sends it, returns sent nbytes on success, on failure returns -1 */
ssize_t send_socket(int sockfd, char *in, int flags) {
    
    ssize_t nbytes = 0;
    char sent[MAX_MSG];
    
    memset(sent, '\0', MAX_MSG);
    //strncat(sent, in, MAX_MSG - 1);        // TODO: MAX_MSG - 2 Enforce \n
                                            // TODO: strip user added \n's

    // instead of strncat use a more secure function like snprintf()
    int written = snprintf(sent, MAX_MSG, "%s", in);
    if (written < 0 || written >= MAX_MSG) {
        logfmt(stderr, ERROR, "snprintf errored or truncation\n");
        return -1;
    }

    size_t sent_size = strlen(sent);
    nbytes = send(sockfd, sent, sent_size, flags);

    if (nbytes < 0) {

        if (errno == ECONNRESET) { 
            logfmt(stdout, WARN, "conn %d: forcibly closed the connection\n", sockfd);
        } else {
            perror("send");
        }
        return -1;
    } else if (nbytes == 0) {
        return -1;
    }

    printf("sent %ld bytes to %d\n", nbytes, sockfd);
    printf("raw: %s", sent);

    return nbytes;
}

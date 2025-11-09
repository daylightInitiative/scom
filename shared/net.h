#ifndef SCOM_SHARED_NET_H
#define SCOM_SHARED_NET_H

#define MAX_PORT_LEN 10 // 65535
#define HOSTPORT 4444
#define MAX_MSG 512


struct ipstr {
    char address[INET_ADDRSTRLEN];
    char port[MAX_PORT_LEN + 1];
};

// func prototypes
void *get_in_addr(struct sockaddr *sa);
struct ipstr get_ip_str(struct sockaddr_storage *ss);

// read_socket and send_socket have slightly different implementations, so omitting them here

#endif
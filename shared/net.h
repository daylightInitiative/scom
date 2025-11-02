#ifndef SCOM_SHARED_NET_H
#define SCOM_SHARED_NET_H

#define MAX_PORT_LEN 10 // 65535
#define HOSTPORT 4444
#define MAX_MSG 512


struct ipstr {
    char address[INET_ADDRSTRLEN];
    char port[MAX_PORT_LEN + 1];
};


struct clientopts {
 
    sa_family_t family;     // address family  AF_INET, AF_INET6
    in_port_t port;         // server port     uint16_t     
    struct sockaddr_storage caddr; // TODO: migrate to a caddr for ipv4/ipv6 full support

    FILE *logfile;          // can be stdin, file, or if UNSPEC: (syslog)
    int verbose;            // verbosity enabled
};

// func prototypes
void *get_in_addr(struct sockaddr *sa);
struct ipstr get_ip_str(struct sockaddr_storage *ss);

// read_socket and send_socket have slightly different implementations, so omitting them here

#endif
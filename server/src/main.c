
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/epoll.h>
#include <unistd.h>

#include "server.h"


// TODO: refactor parse_network_args to server.c


void usage(int status) {

    puts("\nUsage: scomd [ OPTIONS... ] [ FILE... ]\n");
    puts("\nOptions: ");
    puts(" -4\tforces scomd to use IPv4 addresses only.");
    puts(" -6\tforces scomd to use IPv6 addresses only.");
    puts(" -e\twrite debug logs to standard error instead of the system log.");
    puts(" -p\tspecify what port to listen on");
    puts(" -h\tdisplay this help message");
    puts(" -v\tproduce more verbose output");

    exit(status);
}



int parse_network_args(int argc, char **argv, struct serveropts *svopts) {

    //memset(svopts, 0, sizeof(struct serveropts));

    int c;

    /* getopts:

        -4      support only ipv4 connections
        -6      support only ipv6 connections
        -E      write debug logs to selected file instead of stderr
        -e      write debug logs to standard error instead of the system log.
        -p      set server to listen on <PORT>
        -v      set higher verbosity
        -h      displays this help message


    */

    svopts->backlog = 20; // define in server.h default max conn
    svopts->family = AF_INET; // change to AF_UNSPEC later?
    svopts->port = HOSTPORT;
    svopts->logfile = NULL;

    while ((c = getopt(argc, argv, "46Eevh:p:")) != -1) {

        switch (c) {

            case '4':
                svopts->family = AF_INET;
                break;

            case '6':
                svopts->family = AF_INET6;
                break;

            case 'E':
                fprintf(stdout, "Logging to %s\n", optarg);
                // TODO: check access(), if not stdin, close(logfile) [verify file]

                /*  TODO: add logging library, set log mode by verbosity
                // TODO: echo to stdout and logfile bitmask
                if (!file_exists) {
                    svopts->logfile = (FILE *)fopen(optarg, "w");

                    if (svopts->logfile 
                }*/

                break;

            case 'e':
                svopts->logfile = stderr;
                break;

            case 'v':
                svopts->verbose = 1;
                break;

            case 'h':
                usage(EXIT_SUCCESS);
                break;

            case 'p':

                int port = atoi(optarg);

                // undefined behavior: atoi fails and port equals zero

                // If its under 1034, need root
                // If its greater than 1034, and not more than 65,535

                if (port < 1034) {
                    // FAIL, requires root privilages
                    fprintf(stderr, "Ports <= 1034 non-inclusive require elevated privilages\n");
                    return -1;
                } else if (port >= 65535) {
                    fprintf(stderr, "Port must be between 0 and 65535 inclusive\n");
                    return -1;
                } else {
                    fprintf(stdout, "changing port to %s\n", optarg);
                    svopts->port = port;
                }

                break;

            case '?':
                usage(EXIT_FAILURE);
                break;

            default:
                exit(EXIT_FAILURE);
        }
    }

    /* remove getopt options */
    argc -= optind;
    argv += optind;

    /* . . . */

    return 0;
}





int main(int argc, char **argv) {

    int status = -1;
    int ret = -1;   

    struct serveropts svopts = {0};
    struct server srv = {0}; 

    ret = parse_network_args(argc, argv, &svopts);

    if (ret < 0) {
        fprintf(stderr, "Failure to parse network arguments\n");
        //goto FAILURE; ??
        exit(EXIT_FAILURE);
    }
    
    /* initialize server */
    status = init_server(&srv, &svopts);

    if (status < 0) {
        fprintf(stderr, "Failure to initialize server\n");
        exit(EXIT_FAILURE);
    }

    while (1) {

       poll_server(&srv, &svopts, 1000); 

    }

    // man 3p shutdown

    close(srv.sockfd);
    if (close(srv.epollfd)) {
        perror("close"); // more specific errors?
    }

    return 0;
}



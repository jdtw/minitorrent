#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include "connections.h"

int client_connect(char *host, char *port) {
    struct addrinfo hints;
    struct addrinfo *result;
    int sfd;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if(getaddrinfo(host, port, &hints, &result) < 0) {
        return -1;
    }
    sfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sfd < 0) {
        return -1;
    }
    if(connect(sfd, result->ai_addr, result->ai_addrlen) < 0) {
        return -1;
    }
    freeaddrinfo(result);
    return sfd;
}

void getmyaddr(char *addr) {
    char hostname[HOST_MAX];
    struct hostent *hp;
    if(gethostname(hostname, HOST_MAX) < 0) {
        exit(0);
    }
    hp = gethostbyname(hostname);
    strcpy(addr, inet_ntoa(*(struct in_addr*)hp->h_addr_list[0]));
}


int server_listen(char *port) {
    char host[HOST_MAX];
    struct addrinfo hints;
    struct addrinfo *result;
    int sfd, s;

    getmyaddr(host);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    s = getaddrinfo(host, port, &hints, &result);
    if (s < 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(0);
    }
    sfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sfd < 0) {
        perror("socket");
        exit(0);
    }
    if(bind(sfd, result->ai_addr, result->ai_addrlen) < 0) {
        perror("bind");
        exit(0);
    }
    if(listen(sfd, MAXBACKLOG) < 0) {
        perror("listen");
        exit(0);
    }
    freeaddrinfo(result);
    return sfd;
}


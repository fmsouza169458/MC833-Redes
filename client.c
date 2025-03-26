#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define PORT "3490"  // A porta em que o cliente irá se conectar
#define BUFFER_SIZE 5000

// Recupera endereço, IPv4 ou IPv6
void *get_in_addr(struct sockaddr *sa) {
    if(sa ->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void receive_full_message(int sockfd) {
    char buffer[BUFFER_SIZE];
    int numbytes;

    while ((numbytes = recv(sockfd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[numbytes] = '\0';
        printf("%s", buffer);  // Imprime parte da resposta
    }

    if (numbytes == -1) {
        perror("recv");
    }

    printf("\n");
}

int main(int argc, char *argv[]) {
    int sockfd;
    int status;
    struct addrinfo hints, *servinfo, *p;
    char s[INET6_ADDRSTRLEN];

    if (argc < 3) {
        fprintf(stderr, "usage: client hostname operation [params]\n");
        exit(1);
    }
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "Erro em getaddrinfo: %s \n", gai_strerror(status));
        return 1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: falha em conectar\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr*)p->ai_addr), s, sizeof s);
    printf("cliente: conectado a %s\n", s);

    freeaddrinfo(servinfo);

    char request[BUFFER_SIZE];
    snprintf(request, sizeof(request), "%s:%s", argv[2], argc > 3 ? argv[3] : "");
    for (int i = 4; i < argc; i++) {
        strncat(request, ",", sizeof(request) - strlen(request) - 1);
        strncat(request, argv[i], sizeof(request) - strlen(request) - 1);
    }

    if (send(sockfd, request, strlen(request), 0) == -1) {
        perror("send");
        exit(1);
    }

    // Recebendo a resposta completa
    receive_full_message(sockfd);

    close(sockfd);

    return 0;
}
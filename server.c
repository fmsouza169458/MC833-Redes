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
#include <uuid/uuid.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "3490"  // A porta em que os usuários se conectarão
#define BACKLOG 10   // Quantas conexões pendentes serão mantidas na fila
#define BUFFER_SIZE 5000
#define FILENAME "filmes.csv"

// Função para enviar mensagens de texto através de um socket
void send_message(int sockfd, const char *message) {
    size_t len = strlen(message);
    ssize_t total_sent = 0;
    while (total_sent < len) {
        ssize_t sent = send(sockfd, message + total_sent, len - total_sent, 0);
        if (sent == -1) {
            perror("Erro ao enviar mensagem");
            return;
        }
        total_sent += sent;
    }
}

// Função para lidar com os filhos zumbis
void sigchld_handler(int s) {
    (void)s; // Evitar o warning sobre argumento não utilizado
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

// Recupera endereço, IPv4 ou IPv6
void *get_in_addr(struct sockaddr *sa) {
    if(sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// Função para adicionar um novo filme
void write_movie(const char* params, int sockfd) {
    FILE *file = fopen(FILENAME, "a");
    if (file == NULL) {
        perror("Erro ao abrir o arquivo");
        send_message(sockfd, "Erro ao abrir o arquivo.\n");
        return;
    }

    uuid_t uuid;
    uuid_generate_random(uuid);
    char uuidtext[37]; // 36 caracteres + '\0' para o terminador nulo
    uuid_unparse(uuid, uuidtext);

    fprintf(file, "%s,%s\n", uuidtext, params);
    fclose(file);
    send_message(sockfd, "Filme adicionado com sucesso.\n");
}

// Função para adicionar um novo gênero ao filme
void add_genre(const char* params, int sockfd) {
    char *params_copy = strdup(params);
    if (params_copy == NULL) {
        send_message(sockfd, "Erro ao alocar memória.\n");
        return;
    }

    char *id = strtok(params_copy, ",");
    char *new_genre = strtok(NULL, ",");

    if (id == NULL || new_genre == NULL) {
        send_message(sockfd, "Parâmetros inválidos.\n");
        free(params_copy);
        return;
    }

    FILE *file = fopen(FILENAME, "r");
    FILE *tmp = fopen("tmp_filmes.csv", "w");

    if (file == NULL || tmp == NULL) {
        perror("Erro ao abrir o arquivo");
        send_message(sockfd, "Erro ao abrir o arquivo.\n");
        free(params_copy);
        return;
    }

    char line[256];
    int found = 0;
    while (fgets(line, sizeof(line), file)) {
        char *csv_id = strtok(line, ",");
        char *remainder = strtok(NULL, "\n");

        if (strcmp(id, csv_id) == 0) {
            found = 1;
            fprintf(tmp, "%s,%s,%s\n", csv_id, new_genre, remainder);
        } else {
            fprintf(tmp, "%s,%s\n", csv_id, remainder);
        }
    }

    fclose(file);
    fclose(tmp);
    rename("tmp_filmes.csv", FILENAME);
    free(params_copy);

    if (found) {
        send_message(sockfd, "Gênero atualizado com sucesso.\n");
    } else {
        send_message(sockfd, "Filme não encontrado.\n");
    }
}

// Função para remover um filme pelo identificador
void remove_movie(const char* id, int sockfd) {
    FILE *file = fopen(FILENAME, "r");
    FILE *tmp = fopen("tmp_filmes.csv", "w");

    if (file == NULL || tmp == NULL) {
        perror("Erro ao abrir o arquivo");
        send_message(sockfd, "Erro ao abrir o arquivo.\n");
        return;
    }

    char line[256];
    int found = 0;
    while (fgets(line, sizeof(line), file)) {
        char *csv_id = strtok(line, ",");
        char *remainder = strtok(NULL, "\n");

        if (strcmp(id, csv_id) != 0) {
            fprintf(tmp, "%s,%s\n", csv_id, remainder);
        } else {
            found = 1;
        }
    }

    fclose(file);
    fclose(tmp);
    rename("tmp_filmes.csv", FILENAME);

    if (found) {
        send_message(sockfd, "Filme removido com sucesso.\n");
    } else {
        send_message(sockfd, "Filme não encontrado.\n");
    }
}

// Função para listar todos os títulos de filmes com seus identificadores
void list_movie_titles(int sockfd) {
    FILE *file = fopen(FILENAME, "r");
    if (file == NULL) {
        perror("Erro ao abrir o arquivo");
        send_message(sockfd, "Erro ao abrir o arquivo.\n");
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char *id = strtok(line, ",");
        char *title = strtok(NULL, ",");

        if (title != NULL) {
            send_message(sockfd, id);
            send_message(sockfd, ": ");
            send_message(sockfd, title);
            send_message(sockfd, "\n");
        }
    }
    fclose(file);
}

// Função para listar informações de todos os filmes
void list_all_movies(int sockfd) {
    FILE *file = fopen(FILENAME, "r");
    if (file == NULL) {
        perror("Erro ao abrir o arquivo");
        send_message(sockfd, "Erro ao abrir o arquivo.\n");
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        send_message(sockfd, line);
    }
    fclose(file);
}

// Função para listar informações de um filme específico
void list_movie_details(const char* id, int sockfd) {
    FILE *file = fopen(FILENAME, "r");
    if (file == NULL) {
        perror("Erro ao abrir o arquivo");
        send_message(sockfd, "Erro ao abrir o arquivo.\n");
        return;
    }

    char line[256];
    int found = 0;
    while (fgets(line, sizeof(line), file)) {
        char *csv_id = strtok(line, ",");
        if (strcmp(id, csv_id) == 0) {
            found = 1;
            send_message(sockfd, line);
            break;
        }
    }
    fclose(file);

    if (!found) {
        send_message(sockfd, "Filme não encontrado.\n");
    }
}

// Função para listar todos os filmes de um determinado gênero
void list_movies_by_genre(const char* genre, int sockfd) {
    FILE *file = fopen(FILENAME, "r");
    if (file == NULL) {
        perror("Erro ao abrir o arquivo");
        send_message(sockfd, "Erro ao abrir o arquivo.\n");
        return;
    }

    char line[256];
    int found_some = 0;
    while (fgets(line, sizeof(line), file)) {
        char *csv_id = strtok(line, ",");
        strtok(NULL, " "); // Ignora o espaço após a vírgula
        char *csv_genre = strtok(NULL, ",");

        if (csv_genre != NULL && strcmp(genre, csv_genre) == 0) {
            found_some = 1;
            send_message(sockfd, line);
        }
    }
    fclose(file);

    if (!found_some) {
        send_message(sockfd, "Nenhum filme encontrado para o gênero especificado.\n");
    }
}

// Função para lidar com a requisição
void handle_request(int new_fd) {
    char buffer[BUFFER_SIZE];
    int numbytes;

    if ((numbytes = recv(new_fd, buffer, sizeof(buffer) - 1, 0)) == -1) {
        perror("recv");
        close(new_fd);
        exit(1);
    }

    buffer[numbytes] = '\0';
    char *operation = strtok(buffer, ":");
    char *params = strtok(NULL, ":");

    if (strcmp(operation, "write") == 0) {
        write_movie(params, new_fd);
    } else if (strcmp(operation, "add_genre") == 0) {
        add_genre(params, new_fd);
    } else if (strcmp(operation, "remove") == 0) {
        remove_movie(params, new_fd);
    } else if (strcmp(operation, "list_titles") == 0) {
        list_movie_titles(new_fd);
    } else if (strcmp(operation, "list_all") == 0) {
        list_all_movies(new_fd);
    } else if (strcmp(operation, "list_details") == 0) {
        list_movie_details(params, new_fd);
    } else if (strcmp(operation, "list_by_genre") == 0) {
        list_movies_by_genre(params, new_fd);
    } else {
        send_message(new_fd, "Operação desconhecida.\n");
    }

    close(new_fd);
}

// Função principal
int main() {
    int sockfd, new_fd;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr;
    socklen_t sin_size;
    struct sigaction sa;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    // Configuração do endereço
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // Loop para encontrar a melhor maneira de criar um socket
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo);

    if (p == NULL) {
        fprintf(stderr, "server: falha ao realizar bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: aguardando por conexões...\n");

    while (1) {
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
        printf("server: conexão com %s\n", s);

        if (!fork()) {
            close(sockfd);
            handle_request(new_fd);
            close(new_fd);
            exit(0);
        }
        close(new_fd);
    }

    return 0;
}
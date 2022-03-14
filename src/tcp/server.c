#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFFER_LENGTH 65536
#define BACKLOG SOMAXCONN

void sigchld_handler(int arg) {
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0) {
        continue;
    }
    errno = saved_errno;
}

const void *get_in_addr(const struct sockaddr *addr) {
    if (addr->sa_family == AF_INET) {
        return &(((const struct sockaddr_in *)addr)->sin_addr);
    }
    return &(((const struct sockaddr_in6 *)addr)->sin6_addr);
}

in_port_t get_in_port(const struct sockaddr *addr) {
    if (addr->sa_family == AF_INET) {
        return ntohs(((const struct sockaddr_in *)addr)->sin_port);
    }
    return ntohs(((const struct sockaddr_in6 *)addr)->sin6_port);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *port = argv[1];
    struct addrinfo hints = {.ai_flags = AI_PASSIVE,
                             .ai_family = AF_UNSPEC,
                             .ai_socktype = SOCK_STREAM};
    struct addrinfo *addr_info_head;

    {
        int gai_status = getaddrinfo(NULL, port, &hints, &addr_info_head);
        if (gai_status != 0) {
            fprintf(stderr, "failed to get port info: %s\n",
                    gai_strerror(gai_status));
            exit(EXIT_FAILURE);
        }
    }

    int s;
    struct addrinfo *addr_info;
    for (addr_info = addr_info_head; addr_info != NULL;
         addr_info = addr_info->ai_next) {
        s = socket(addr_info->ai_family, addr_info->ai_socktype,
                   addr_info->ai_protocol);
        if (s == -1) {
            continue;
        }
        {
            int yes = 1;
            if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) ==
                -1) {
                perror("failed to configure socket");
                freeaddrinfo(addr_info_head);
                close(s);
                exit(EXIT_FAILURE);
            }
        }
        if (bind(s, addr_info->ai_addr, addr_info->ai_addrlen) == -1) {
            close(s);
            continue;
        }
        break;
    }
    if (addr_info == NULL) {
        perror("failed to bind socket");
        freeaddrinfo(addr_info_head);
        exit(EXIT_FAILURE);
    }
    char addr_ip[INET6_ADDRSTRLEN];
    inet_ntop(addr_info->ai_family, get_in_addr(addr_info->ai_addr), addr_ip,
              sizeof addr_ip);
    freeaddrinfo(addr_info_head);

    struct sigaction action = {.sa_handler = sigchld_handler,
                               .sa_flags = SA_RESTART};
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGCHLD, &action, NULL) == -1) {
        perror("failed to set up signal handler");
        close(s);
        exit(EXIT_FAILURE);
    }

    if (listen(s, BACKLOG) == -1) {
        perror("failed to listen");
        close(s);
        exit(EXIT_FAILURE);
    }

    printf("listening for incoming messages at %s port %s...\n", addr_ip, port);

    for (;;) {
        struct sockaddr_storage from_addr;
        socklen_t from_addr_len = sizeof from_addr;
        int connection =
            accept(s, (struct sockaddr *)&from_addr, &from_addr_len);
        if (connection == -1) {
            perror("failed to accept connection");
            continue;
        }

        char from_addr_ip[INET6_ADDRSTRLEN];
        inet_ntop(from_addr.ss_family,
                  get_in_addr((const struct sockaddr *)&from_addr),
                  from_addr_ip, sizeof from_addr_ip);
        in_port_t from_addr_port =
            get_in_port((const struct sockaddr *)&from_addr);
        printf("got connection from %s port %d\n", from_addr_ip,
               from_addr_port);

        if (!fork()) {
            close(s);

            char buffer[BUFFER_LENGTH];
            int bytes_received = recv(connection, buffer, sizeof buffer - 1, 0);
            if (bytes_received == -1) {
                perror("failed to receive message");
                close(connection);
                exit(EXIT_FAILURE);
            }
            buffer[bytes_received] = '\0';
            printf("received %d bytes from %s port %d:\n", bytes_received,
                   from_addr_ip, from_addr_port);
            printf("%s\n", buffer);

            int bytes_sent = send(connection, buffer, bytes_received, 0);
            if (bytes_sent == -1) {
                perror("failed to send message");
                close(connection);
                exit(EXIT_FAILURE);
            }
            printf("sent %d bytes to %s port %d:\n", bytes_sent, from_addr_ip,
                   from_addr_port);
            printf("%s\n", buffer);

            close(connection);

            printf("closed connection to %s port %d\n", from_addr_ip,
                   from_addr_port);
            exit(EXIT_SUCCESS);
        } else {
            close(connection);
        }
    }

    close(s);

    return 0;
}

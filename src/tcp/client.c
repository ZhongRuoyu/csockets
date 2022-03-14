#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_LENGTH 65536

const void *get_in_addr(const struct sockaddr *addr) {
    if (addr->sa_family == AF_INET) {
        return &(((const struct sockaddr_in *)addr)->sin_addr);
    }
    return &(((const struct sockaddr_in6 *)addr)->sin6_addr);
}

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <hostname> <port> <message>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *hostname = argv[1];
    const char *port = argv[2];
    const char *message = argv[3];
    struct addrinfo hints = {.ai_family = AF_UNSPEC,
                             .ai_socktype = SOCK_STREAM};
    struct addrinfo *addr_info_head;

    {
        int gai_status = getaddrinfo(hostname, port, &hints, &addr_info_head);
        if (gai_status != 0) {
            fprintf(stderr, "failed to get host info: %s\n",
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
        if (connect(s, addr_info->ai_addr, addr_info->ai_addrlen) == -1) {
            close(s);
            continue;
        }
        break;
    }
    if (addr_info == NULL) {
        perror("failed to connect");
        exit(EXIT_FAILURE);
    }
    char addr_ip[INET6_ADDRSTRLEN];
    inet_ntop(addr_info->ai_family, get_in_addr(addr_info->ai_addr), addr_ip,
              sizeof addr_ip);
    freeaddrinfo(addr_info_head);

    printf("connected to %s (%s) port %s\n", hostname, addr_ip, port);

    int bytes_sent = send(s, message, strlen(message), 0);
    if (bytes_sent == -1) {
        perror("failed to send message");
        close(s);
        exit(EXIT_FAILURE);
    }
    printf("sent %d bytes to %s:\n", bytes_sent, hostname);
    printf("%s\n", message);

    char buffer[BUFFER_LENGTH];
    int bytes_received = recv(s, buffer, sizeof buffer - 1, 0);
    if (bytes_received == -1) {
        perror("failed to receive message");
        close(s);
        exit(EXIT_FAILURE);
    }
    buffer[bytes_received] = '\0';
    printf("received %d bytes from %s:\n", bytes_received, hostname);
    printf("%s\n", buffer);

    close(s);

    return 0;
}

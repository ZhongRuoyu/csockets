#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <hostname> <port> <message>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *hostname = argv[1];
    const char *port = argv[2];
    const char *message = argv[3];
    struct addrinfo hints = {.ai_family = AF_UNSPEC, .ai_socktype = SOCK_DGRAM};
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
        if (s != -1) {
            break;
        }
    }
    if (addr_info == NULL) {
        fprintf(stderr, "failed to create socket\n");
        exit(EXIT_FAILURE);
    }

    int bytes_sent = sendto(s, message, strlen(message), 0, addr_info->ai_addr,
                            addr_info->ai_addrlen);
    if (bytes_sent == -1) {
        perror("failed to send message");
        freeaddrinfo(addr_info_head);
        close(s);
        exit(EXIT_FAILURE);
    }
    printf("sent %d bytes to %s\n", bytes_sent, hostname);

    freeaddrinfo(addr_info_head);
    close(s);

    return 0;
}

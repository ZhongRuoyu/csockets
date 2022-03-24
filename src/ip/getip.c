#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <hostname>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *hostname = argv[1];
    struct addrinfo hints = {.ai_family = AF_UNSPEC,
                             .ai_socktype = SOCK_STREAM};
    struct addrinfo *addr_info_head;

    {
        int gai_status = getaddrinfo(hostname, NULL, &hints, &addr_info_head);
        if (gai_status != 0) {
            fprintf(stderr, "failed to get host info: %s\n",
                    gai_strerror(gai_status));
            exit(EXIT_FAILURE);
        }
    }

    for (struct addrinfo *p = addr_info_head; p != NULL; p = p->ai_next) {
        const void *addr;
        const char *ip_version;
        char ip_addr[INET6_ADDRSTRLEN];

        if (p->ai_family == AF_INET) {
            ip_version = "IPv4";
            const struct sockaddr_in *ipv4 =
                (const struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
        } else {
            ip_version = "IPv6";
            const struct sockaddr_in6 *ipv6 =
                (const struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
        }

        inet_ntop(p->ai_family, addr, ip_addr, sizeof ip_addr);
        printf("%s: %s\n", ip_version, ip_addr);
    }

    freeaddrinfo(addr_info_head);

    return 0;
}

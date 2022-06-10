#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#define PORT "4444"
#define BACKLOG 10

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void run_server(unsigned int port)
{
    int sockfd, new_fd;
    struct addrinfo hints, *res;
    struct sockaddr_storage their_addr;
    socklen_t sin_size;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, PORT, &hints, &res);

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    bind(sockfd, res->ai_addr, res->ai_addrlen);

    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("server: waiting for connections...\n");

    while(1)
    {
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1)
        {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
        printf("server: got connection from %s\n", s);

        if (!fork())
        {
            close(sockfd);
            char response[500] = "HTTP/1.0 200 OK\nServer: httpc 0.0.1\nMIME-version: 1.0\nContent-Type: text/html\n\n";
            if (send(new_fd, response, strlen(response), 0) == -1)
            {
                perror("send");
            }
            char data[1024];
            FILE *fp;
            fp = fopen("index.html", "r");
            while (fgets(data, 1024, fp) != NULL)
            {
                if (send(new_fd, data, strlen(data), 0) == -1)
                {
                    perror("send file");
                }
            }
            int status;
            char buf[512];
            do {
                status = recv(new_fd, buf, sizeof buf, MSG_DONTWAIT);
            } while (status > 0);

            close(new_fd);
            exit(EXIT_SUCCESS);
        }
        close(new_fd);
    }
}

int main(int argc, char **argv)
{
    unsigned int port;

    if (argc == 1)
    {
        port = 80;
    }
    else if (argc == 2)
    {
        port = atoi(argv[1]);
    }
    else
    {
        printf("%s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    run_server(port);

    return EXIT_SUCCESS;
}

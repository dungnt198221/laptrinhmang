#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

int main()
{
    int receiver = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9000);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)))
    {
        perror("bind() failed");
        return 1;
    }

    if (listen(listener, 5))
    {
        perror("listen() failed");
        return 1;
    }

    fd_set fdread, fdtest;
    FD_ZERO(&fdread);
    FD_SET(listener, &fdread);

    char buf[256];
    struct timeval tv;

    while (1)
    {
        fdtest = fdread;

        tv.tv_usec = 0;
        tv.tv_sec = 5;

        int ret = select(FD_SETSIZE, &fdtest, NULL, NULL, &tv);

        if (ret < 0)
        {
            perror("select() failed");
            break;
        }

        if (ret == 0)
        {
            printf("Timed out.\n");
            continue;
        }

        for (int i = 0; i < FD_SETSIZE; i++)
            if (FD_ISSET(i, &fdtest))
            {
                if (i == listener)
                {
                    // Chap nhan ket noi
                    int client = accept(listener, NULL, NULL);
                    if (client < FD_SETSIZE)
                    {
                        printf("New client connected: %d\n", client);
                        FD_SET(client, &fdread);
                    }
                    else
                    {
                        printf("Too many connections.\n");
                        close(client);
                    }
                }
                else
                {
                    // Nhan du lieu
                    int ret = recv(i, buf, sizeof(buf), 0);
                    if (ret <= 0)
                    {
                        printf("Client %d disconnected.\n", i);
                        close(i);
                        FD_CLR(i, &fdread);
                    }
                    else
                    {
                        buf[ret] = 0;
                        printf("Received from %d: %s\n", i, buf);

                        
                        while (1)
                        {
                            int ret = recvfrom(receiver, buf, sizeof(buf), 0,
                                               NULL, NULL);
                            if (ret <= 0)
                            {
                                break;
                            }

                            buf[ret] = 0;
                            printf("%d bytes received: %s\n", ret, buf);
                        }
                    }
                }
            }
    }

    close(listener);

    return 0;
}
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX_CLIENTS 64

int main()
{
    int listener = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9000);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) 
    {
        perror("bind() failed");
        return 1;
    }

    if (listen(listener, MAX_CLIENTS)) 
    {
        perror("listen() failed");
        return 1;
    }

    fd_set fdread, fdtest;
    FD_ZERO(&fdread);
    FD_SET(listener, &fdread);

    int client_sockets[MAX_CLIENTS];
    int num_clients = 0;

    char buf[256];

    while (1)
    {
        fdtest = fdread;

        int max_fd = listener;
        for (int i = 0; i < num_clients; i++)
        {
            if (client_sockets[i] > max_fd)
                max_fd = client_sockets[i];
            FD_SET(client_sockets[i], &fdtest);
        }

        int ret = select(max_fd + 1, &fdtest, NULL, NULL, NULL);

        if (ret < 0)
        {
            perror("select() failed");
            break;
        }

        if (FD_ISSET(listener, &fdtest))
        {
            // Chấp nhận kết nối từ client mới
            int client = accept(listener, NULL, NULL);
            if (client < 0)
            {
                perror("accept() failed");
                break;
            }

            if (num_clients < MAX_CLIENTS)
            {
                client_sockets[num_clients++] = client;
                printf("New client connected: %d\n", client);
                sprintf(buf, "Xin chao. Hien co %d clients dang ket noi.", num_clients);
                write(client, buf, strlen(buf));
            }
            else
            {
                printf("Too many connections.\n");
                close(client);
            }
        }

        for (int i = 0; i < num_clients; i++)
        {
            if (FD_ISSET(client_sockets[i], &fdtest))
            {
                // Nhận dữ liệu từ client
                int ret = read(client_sockets[i], buf, sizeof(buf) - 1);
                if (ret <= 0)
                {
                    printf("Client %d disconnected.\n", client_sockets[i]);
                    close(client_sockets[i]);

                    // Xoá socket của client khỏi danh sách
                    for (int j = i; j < num_clients - 1; j++)
                        client_sockets[j] = client_sockets[j + 1];
                    num_clients--;
                }
                else
                {
                    buf[ret] = '\0';

                    // Chuẩn hóa xâu ký tự
                    for (int j = 0; j < strlen(buf); j++)
                    {
                        if (buf[j] == ' ')
                        {
                            // Xóa ký tự dư thừa
                            int k = j + 1;
                            while (buf[k] == ' ')
                                k++;
                            for (; k < strlen(buf); k++)
                                buf[k - 1] = buf[k];
                            buf[k - 1] = '\0';
                        }
                        else if (j == 0 || buf[j - 1] == ' ')
                        {
                            // Chuyển các ký tự đầu từ thành chữ thường
                            if (buf[j] >= 'A' && buf[j] <= 'Z')
                                buf[j] = buf[j] - 'A' + 'a';
                        }
                    }

                    // Trả kết quả về client
                    write(client_sockets[i], buf, strlen(buf));

                    // Kiểm tra điều kiện thoát
                    if (strcmp(buf, "exit") == 0)
                    {
                        close(client_sockets[i]);

                        // Xoá socket của client khỏi danh sách
                        for (int j = i; j < num_clients - 1; j++)
                            client_sockets[j] = client_sockets[j + 1];
                        num_clients--;

                        break;
                    }
                }
            }
        }
    }
    
    close(listener);    

    return 0;
}

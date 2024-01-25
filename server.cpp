#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <vector>
#define MAX_EVENTS 10

int main()
{

    std::vector<int> clientSockets;
    char username[12];
    char msg[1024];
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockfd != -1);

    struct sockaddr_in saddr, caddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = PF_INET;
    saddr.sin_port = htons(6000);
    saddr.sin_addr.s_addr = INADDR_ANY;

    int res = bind(sockfd, (struct sockaddr *)&saddr, sizeof(saddr));
    assert(res != -1);

    listen(sockfd, 5);

    int epollfd = epoll_create1(0);
    assert(epollfd != -1);

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = sockfd;

    res = epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev);
    assert(res != -1);

    while (1)
    {
        struct epoll_event events[MAX_EVENTS];
        int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);

        for (int i = 0; i < nfds; ++i)
        {
            if (events[i].data.fd == sockfd)
            {
                socklen_t len = sizeof(caddr);
                int c = accept(sockfd, (struct sockaddr *)&caddr, &len);
                if (c < 0)
                {
                    perror("accept");
                    continue;
                }
                printf("Accepted connection from %s\n", inet_ntoa(caddr.sin_addr));

                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = c;
                res = epoll_ctl(epollfd, EPOLL_CTL_ADD, c, &ev);
                assert(res != -1);
                clientSockets.push_back(c);
            }
            else
            {
                int c = events[i].data.fd;
                char buf[128] = {0};
                int n = recv(c, buf, sizeof(buf) - 1, 0);
                char temp[128];
                strcpy(temp,buf);
                printf("buf,strlen:%d",strlen(buf));
                if (n <= 0)
                {
                    // Connection closed or error
                    if (n == 0)
                    {
                        printf("Connection closed by client\n");
                    }
                    else
                    {
                        perror("recv");
                    }
                    close(c);
                    epoll_ctl(epollfd, EPOLL_CTL_DEL, c, NULL);
                }
                else
                {
                    printf("Received message from %s: %s\n", inet_ntoa(caddr.sin_addr), "ok");

                    // Splitting the buf content based on ##
                    char *token;
                    char *rest = buf;
                    char *substrings[3];  // Array to store the three substrings

                    for (int i = 0; i < 3; i++) {
                        token = strtok_r(rest, "##", &rest);
                        if (token == NULL) {
                            // Handle the case where there are fewer than three substrings
                            printf("Error: Less than three substrings found.\n");
                            break;
                        }
                        substrings[i] = token;
                    }

                    // Print the three substrings
                    for (int i = 0; i < 3; i++) {
                        printf("Substring %d: %s\n", i + 1, substrings[i]);
                    }
                    if ((strcmp(substrings[0], "msg") == 0 ))
                    {
                        // send(c, substrings[2], strlen(substrings[2])+1, 0);
                        printf("send number is%s\n",substrings[2]);
                        strcpy(username,substrings[1]);
                        printf("username %s\n", username);
                        strcpy(msg,substrings[2]);
                        for (int cli : clientSockets)
                        {
                            if (cli!= c)
                            {
                                send(cli, temp, strlen(temp)+1, 0);
                            }   
                        }
                        
                    }else
                    {
                        int filesize = atoi(substrings[1]);
                        printf("Received file size: %d bytes\n", filesize);
                        int first_buf_file_len = 128 - 2*strlen("##") -strlen(substrings[0])-strlen(substrings[1]);
                        printf("first buffer size: %d bytes\n", first_buf_file_len);
                        int recvbyte = first_buf_file_len;
                        FILE *file = fopen("server_file", "w+");
                        fwrite(substrings[2], first_buf_file_len, 1, file);
                        fclose(file);
                        
                        // Check if the expected number of bytes were received
                        while (recvbyte <= filesize) {
                            // Save the file content to a local file
                            char fileContent[1024*2];
                            int bytesRead = recv(c, fileContent, sizeof(fileContent), 0);
                            FILE *file = fopen("server_file", "ab+");
                            fwrite(fileContent, bytesRead, 1, file);
                            fclose(file);
                            recvbyte += bytesRead;
                            printf("已完成%d%\n",recvbyte*100/filesize);
                            printf("recvbyte : %d ,filesize: %d \n",recvbyte,filesize);
                        } 
                        printf("finish transfer size: %d bytes\n", filesize);
                        //转发信息
                        FILE *fileToSend = fopen("server_file", "rb");

                        if (fileToSend == NULL) {
                            perror("打开文件失败");
                            // 处理错误
                            return 1; // 返回错误代码表示失败
                        }
                        char buffer[1024];

                        size_t bytesRead;
                        send(c,"F#\0",sizeof("F#\0"),0);
                        // 使用循环读取文件直到文件末尾
                        while ((bytesRead = fread(buffer, 1, 1024, fileToSend)) > 0) {
                            // 在这里对缓冲区中的数据进行处理
                            // 例如，将数据打印到控制台
                            fwrite(buffer, 1, bytesRead, stdout);
                            send(c,buffer,sizeof(buf),0);
                        }

                        // 检查是否出现错误或者是否已经到达文件末尾
                        if (feof(fileToSend)) {
                            // 文件末尾
                            printf("文件读取完成。\n");
                        } else if (ferror(fileToSend)) {
                            // 读取文件时发生错误
                            perror("读取文件错误");
                        }

                        // 使用完文件后记得关闭它
                        fclose(fileToSend);
                    }
                    
                }
            }
        }
    }

    close(sockfd);

    exit(0);
}

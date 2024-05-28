#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#define HEARBEAT_PORT 8000
#define BROADCAST_IP "255.255.255.255"
#define MAX 50

struct socket hearbeat_s;
char *server_port;

struct socket {
    int fd;
    struct sockaddr_in addr;
};

struct socket make_hearbeat_socket()
{
    struct socket s;
    s.fd = socket(AF_INET, SOCK_DGRAM, 0);
    
    int broadcast = 1, opt = 1;
    setsockopt(s.fd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    setsockopt(s.fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    s.addr.sin_family = AF_INET; 
    s.addr.sin_port = htons(HEARBEAT_PORT);
    s.addr.sin_addr.s_addr = inet_addr(BROADCAST_IP);

    bind(s.fd, (struct sockaddr *)&s.addr, sizeof(s.addr));

    return s;
}

int setup_server(char *server_port) {
    struct sockaddr_in addr;
    int server_fd;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(server_port));
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    
    listen(server_fd, 4);

    return server_fd;
}

int accept_client(int server_fd) {
    int client_fd;
    struct sockaddr_in client_addr;
    int address_len = sizeof(client_addr);
    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t*) &address_len);
    return client_fd;
}

void send_hearbeat()
{
    sendto(hearbeat_s.fd, server_port, strlen(server_port), 0, (struct sockaddr *)&hearbeat_s.addr, sizeof(hearbeat_s.addr));
    alarm(1);
}

int main(int argc, char *argv[])
{
    char reqs_subject[MAX][MAX] = {0};
    char reqs_info[MAX][2*MAX] = {0};
    int cnt = 0, new_socket;
    fd_set master_set, working_set;
    FD_ZERO(&master_set);
    
    if (argc != 2)
    {
        write(2, "bad arguments\n", 15);
        exit(1);
    }

    server_port = argv[1];
    hearbeat_s = make_hearbeat_socket();
    int server_fd = setup_server(server_port);
    FD_SET(server_fd, &master_set);
    int max_sd = server_fd;

    signal(SIGALRM, send_hearbeat);
    alarm(1);
    
    system("clear");
    write(1, "Server is running...\n", 22);

    while (1) {
        working_set = master_set;
        int result_s = select(max_sd + 1, &working_set, NULL, NULL, NULL);
        if (result_s == -1 && errno == EINTR) {
            continue;
        }

        for (int i = 0; i <= max_sd; i++) {
            if (FD_ISSET(i, &working_set)) {
                
                if (i == server_fd) {
                    new_socket = accept_client(server_fd);
                    FD_SET(new_socket, &master_set);
                    if (new_socket > max_sd)
                        max_sd = new_socket;
                    write(1, "New client connected...\n", 25);
                }
                
                else {
                    char buff[2*MAX] = {0}, command[MAX] = {0};

                    int bytes_received = recv(i, buff, sizeof(buff), 0);
                    if (bytes_received == 0) { // EOF
                        write(1, "The client closed.\n", 20);
                        close(i);
                        FD_CLR(i, &master_set);
                        continue;
                    }
                    
                    char *dollar_sign = strchr(buff, '$');
                    int length = dollar_sign - buff;
                    strncpy(command, buff, length);

                    if (strncmp("submit", command, 6) == 0) {
                        char *comma_sign = strchr(buff, ',');
                        int len = comma_sign - (dollar_sign + sizeof(char));
                        strncpy(reqs_subject[cnt], dollar_sign + sizeof(char), len);
                        strcat(reqs_subject[cnt], "\n");
                        strcpy(reqs_info[cnt], dollar_sign + sizeof(char));
                        strcat(reqs_info[cnt], "\n");
                        char *info = reqs_info[cnt];
                        int server_file = open("server.txt", O_WRONLY | O_APPEND | O_CREAT, 0777);
                        write(server_file, info, strlen(info));
                        ++cnt;
                        write(1, "The request was written to the server file:)\n", 46);
                    }    

                    else if (strncmp("receive", command, 7) == 0) {
                        int available = 0;
                        char subject[MAX] = {0};
                        strcpy(subject, dollar_sign + sizeof(char));
                        for (int j = 0; j < cnt; j++)
                            if (strncmp(reqs_info[j], subject, strlen(subject) - sizeof(char)) == 0) {
                                available = 1;
                                write(new_socket, reqs_info[j], strlen(reqs_info[j]));
                                break;
                            }
                        if (!available)
                            write(new_socket, "not available", 14);
                    }

                    else if (strncmp("list", command, 4) == 0) {
                        char list_of_subjects[MAX * MAX] = {0};
                        for (int j = 0; j < cnt; j++)
                            strcat(list_of_subjects, reqs_subject[j]);
                        write(new_socket, list_of_subjects, strlen(list_of_subjects));
                    }
                }
            }
        }
    }
}
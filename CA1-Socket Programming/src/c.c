#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#define SERVER_IP "127.0.0.1"
#define BROADCAST_IP "255.255.255.255"
#define MAX 50
#define TIMEOUT 2

static int REQUEST_ACCEPTED, BROADCASTING;
static char BROADCAST_MSG[MAX], SERVER_PORT[MAX + 1] = {0};
static struct socket BROADCAST_S, CLIENT_S;

struct socket {
    int fd;
    struct sockaddr_in addr;
};

void send_broadcast_msg()
{
    if (REQUEST_ACCEPTED) 
    {
        alarm(0);
        return;
    }

    if (sendto(BROADCAST_S.fd, BROADCAST_MSG, strlen(BROADCAST_MSG), 0, (struct sockaddr *)&BROADCAST_S.addr, sizeof(BROADCAST_S.addr)) > -1)
        if (!BROADCASTING)
        {
            write(1, "The client is now broadcasting...\n", 35);
            BROADCASTING = 1;
        }
    alarm(1);
}

void strcpy_until_newline(char *dest, const char *src) {
    while (*src && *src != '\n') {
        *dest = *src;
        dest++;
        src++;
    }
    *dest = '\0';
}

void strcat_until_newline(char *dest, const char *src) {
    while (*dest)
        dest++;
    strcpy_until_newline(dest, src);
}

void add_the_user_response_to_the_request(char *req_to_server) {
    char buff[MAX] = {0};
    read(STDIN_FILENO, buff, sizeof(buff));
    strcat_until_newline(req_to_server, buff);
    strcat_until_newline(req_to_server, ",");
}

int connect_to_server()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(SERVER_PORT));
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        write(1, "Error in connecting to server:(\n", 33);
        exit(1);
    }
    else
        write(1, "Connected to the server...\n", 28);
    return server_fd;
}

struct socket make_socket(const char *port)
{
    struct socket s;
    s.fd = socket(AF_INET, SOCK_DGRAM, 0);
    
    int broadcast = 1, opt = 1;
    setsockopt(s.fd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    setsockopt(s.fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    s.addr.sin_family = AF_INET; 
    s.addr.sin_port = htons(atoi(port));
    s.addr.sin_addr.s_addr = inet_addr(BROADCAST_IP);

    bind(s.fd, (struct sockaddr *)&s.addr, sizeof(s.addr));

    return s;
}

int doesServerAlive(const char *heartbeat_port_B)
{
    int available = 0;
    
    struct socket s = make_socket(heartbeat_port_B);
    
    socklen_t addr_len = sizeof(s.addr);
    
    struct timeval tv;
    float timeout = TIMEOUT;
    tv.tv_sec = timeout;
    tv.tv_usec = timeout * 1000;
    setsockopt(s.fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    if (recvfrom(s.fd, SERVER_PORT, sizeof(SERVER_PORT), 0, (struct sockaddr *)&s.addr, &addr_len) > -1)
    {
        write(1, "The server is ready to serve...\n", 33);
        available = 1;
    }
    else
        write(2, "The server is under repair:(\n", 30);
    close(s.fd);
    return available;
}

int main(int argc, char *argv[])
{
    char reqs_subject[MAX][MAX] = {0};
    char reqs_info[MAX][2*MAX] = {0};
    int cnt = 0;
    fd_set working_set;
    FD_ZERO(&working_set);
    FD_SET(STDIN_FILENO, &working_set);
    int max_sd;

    alarm(0);
    signal(SIGALRM, send_broadcast_msg);

    if (argc != 4)
    {
        write(2, "bad arguments\n", 15);
        exit(1);
    }

    char *heartbeat_port_B = argv[1];
    char *broadcast_port_A = argv[2];
    char *client_port_M = argv[3];

    BROADCAST_S = make_socket(broadcast_port_A);
    CLIENT_S = make_socket(client_port_M);

    char role;
    while(1)
    {
        char buff[MAX] = {0};
        system("clear");
        write(1, "supervisor/user? ", 18);
        read(0, buff, sizeof(buff));

        if (strncmp("supervisor", buff, 10) == 0) 
        {
            role = 's';
            FD_SET(BROADCAST_S.fd, &working_set);
            max_sd = BROADCAST_S.fd;
            break;
        }
        else if (strncmp("user", buff, 4) == 0)
        {
            role = 'u';
            FD_SET(CLIENT_S.fd, &working_set);
            max_sd = CLIENT_S.fd;
            break;
        }
        else
            write(2, "error, select a role...\n", 25);
    }

    int server_fd, available, error_s = 0, connected = 0;
    REQUEST_ACCEPTED = 1;
    while(1)
    {
        if (!error_s && REQUEST_ACCEPTED)
        {
            system("clear");
            available = doesServerAlive(heartbeat_port_B);
            if (available)
            {
                if (!connected)
                {
                    server_fd = connect_to_server();
                    connected = 1;
                }
                if (role == 'u')
                    write(1, " - submit a new request\n", 25);
                write(1, " - receive an existing request\n", 32);
                if (role == 'u')
                    write(1, "submit/", 8);
                write(1, "receive/refresh? ", 18);
            }
            else
            {
                if (connected)
                {
                    close(server_fd);
                    connected = 0;
                }
                if (role == 'u')
                {
                    write(1, " - receive an existing request\n", 32);
                    write(1, "receive/", 9);
                }
                write(1, "refresh? ", 10);
            }
        }

        int result_s = select(max_sd + 1, &working_set, NULL, NULL, NULL);
        if (result_s == -1 && errno == EINTR) {
            error_s = 1;
            continue;
        }
        error_s = 0;

        if (FD_ISSET(STDIN_FILENO, &working_set))
        {
            char command[MAX] = {0};
            char req_to_server[2*MAX] = {0};
            read(STDIN_FILENO, command, sizeof(command));

            if ((strncmp("refresh", command, 7) == 0))
                continue;

            else if ((strncmp("submit", command, 6) == 0) && role == 'u' && available)
            {
                write(1, "submitting a new request has started...\n", 41);
                strcpy(req_to_server, "submit$");
                write(1, "- Complete the Following -\n", 28);
                write(1, " 1. Subject: ", 14);
                add_the_user_response_to_the_request(req_to_server);
                write(1, " 2. Full name: ", 16);
                add_the_user_response_to_the_request(req_to_server);
                write(1, " 3. Student ID number: ", 24);
                add_the_user_response_to_the_request(req_to_server);
                write(1, " 4. The number of units passed: ", 33);
                add_the_user_response_to_the_request(req_to_server);
                write(1, " 5. The number of units obtained in this semester: ", 52);
                add_the_user_response_to_the_request(req_to_server);
                write(server_fd, req_to_server, strlen(req_to_server));
                write(1, "The request was sent to the server:)\n", 38);
                sleep(3);
            }

            else if ((strncmp("receive", command, 7) == 0) && role == 'u')
            {
                char result[2*MAX] = {0};
                char req_subject[MAX] = {0};           
                write(1, "Receiving an existing request has started...\n", 46);
                write(1, "Enter the subject: ", 20);
                read(0, req_subject, sizeof(req_subject));
                if (available)
                {      
                    strcpy(req_to_server, "receive$");
                    strcat(req_to_server, req_subject);
                    write(server_fd, req_to_server, strlen(req_to_server));
                    write(1, "The request was sent to the server...\n", 39);                 
                    read(server_fd, result, sizeof(result));
                    if (strncmp("not available", result, 13) == 0)
                        write(1, "There is no request with this subject on the server:(\n", 55);
                    else
                    {
                        int client_file = open("client.txt", O_WRONLY | O_APPEND | O_CREAT, 0777);
                        write(client_file, result, strlen(result));
                        write(1, "The request was written to the client file:)\n", 46);
                    }
                    sleep(3);
                }
                else
                {
                    memset(BROADCAST_MSG, 0, sizeof(BROADCAST_MSG));
                    strcpy(BROADCAST_MSG, req_subject);
                    REQUEST_ACCEPTED = 0;
                    BROADCASTING = 0;
                    alarm(1);
                }
            }

            else if ((strncmp("receive", command, 7) == 0) && role == 's' && available)
            {
                char list[MAX*MAX] = {0};
                char req_subject[MAX] = {0}, req_info[2*MAX] = {0};
                write(server_fd, "list$", 6);
                read(server_fd, list, sizeof(list));
                write(1, "- List of Requests -\n", 22);
                write(1, list, strlen(list));
                write(1, "select a request: ", 19);
                read(0, req_subject, sizeof(req_subject));
                strcpy(req_to_server, "receive$");
                strcat(req_to_server, req_subject);
                write(server_fd, req_to_server, strlen(req_to_server));
                write(1, "The request was sent to the server...\n", 39);
                read(server_fd, req_info, sizeof(req_info));
                if (strncmp("not available", req_info, 13) == 0)
                    write(1, "There is no request with this subject on the server:(\n", 55);
                else
                {
                    write(1, "The list of requests was updated.", 34);
                    strcat(reqs_subject[cnt], req_subject);
                    strcat(reqs_info[cnt], req_info);
                    ++cnt;
                }   
                sleep(4);     
            }
        }
    
        else if (FD_ISSET(BROADCAST_S.fd, &working_set))
        {
            char req_subject[MAX] = {0}; 
            socklen_t addr_len = sizeof(BROADCAST_S.addr);
            if (recvfrom(BROADCAST_S.fd, req_subject, sizeof(req_subject), 0, (struct sockaddr *)&BROADCAST_S.addr, &addr_len) > -1)
            {
                int index = -1;
                for (int i = 0; i < cnt; i++)
                    if (strcmp(reqs_subject[i], req_subject) == 0)
                    {
                        index = i;
                        write(1, "\nThe requested subject was found on this supervisor...\n", 55);
                        break;
                    }
                if (index != -1) {
                    if (sendto(CLIENT_S.fd, reqs_info[index], strlen(reqs_info[index]), 0, (struct sockaddr *)&CLIENT_S.addr, sizeof(CLIENT_S.addr)) > -1)
                        write(1, "The requested information has been sent to the  client.\n", 57);
                }
                else
                    write(1, "\nThe requested subject not available on this supervisor:(\n", 58);            
                sleep(4);
            }     
        }

        else if (FD_ISSET(CLIENT_S.fd, &working_set))
        {
            char req_info[2*MAX] = {0};
            socklen_t addr_len = sizeof(CLIENT_S.addr);
            if (recvfrom(CLIENT_S.fd, req_info, sizeof(req_info), 0, (struct sockaddr *)&CLIENT_S.addr, &addr_len) > -1)
            {
                REQUEST_ACCEPTED = 1;
                int client_file = open("client.txt", O_WRONLY | O_APPEND | O_CREAT, 0777);
                write(client_file, req_info, strlen(req_info));
                write(1, "The request was received from the supervisor:)\n", 48);
                sleep(3);
            }
        }
    }
    if (available) close(server_fd);
    close(BROADCAST_S.fd);
    close(CLIENT_S.fd);
}

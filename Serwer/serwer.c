#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <error.h>

#define QUEUE_SIZE 5
#define MAX_GAMES 16
#define MAX_EVENTS 10

enum RESPONSES {ACCEPTED = 0, FULLLOBBY = 1, FULLGAMES = 2, NOTYOURMOVE = 3, DIDNOTSTART = 4, ILLEGALMOVE = 5, FIELDTAKEN = 6, SHOULDIGNORE = 7};
enum TURN {NOTSTARTED, PLAYER1, PLAYER2, END};

struct game_t
{
    char lobbyName[80];
    int player1Socket;
    int timeout1;
    int player2Socket;
    int timeout2;
    int board[8][8];
    int turn;
};

struct thread_data_t
{
    int epoll;
    struct game_t games[MAX_GAMES];
    int size;
};

void* CheckTimeouts(void* t_data)
{
    pthread_detach(pthread_self());
    struct thread_data_t *th_data = (struct thread_data_t*)t_data;
    for (;;)
    {
        for (int i = 0; i < th_data->size; i++)
        {
            if (th_data->games[i].player1Socket != 0)
            {

            }
        }
        sleep(1);
    }
    
    
}

void* ThreadBehavior(void* t_data)
{
    pthread_detach(pthread_self());
    struct thread_data_t *th_data = (struct thread_data_t*)t_data;

    struct epoll_event ev[MAX_EVENTS];
    for(;;) {
        int val = epoll_wait(th_data->epoll, (struct epoll_event*)&ev, MAX_EVENTS, -1); 
        for (int i = 0; i < val; i++)
        {
            if (ev->events & EPOLLIN)
            {
                int found = -1;
                for (int j = 0; j < th_data->size; j++)
                {
                    if (th_data->games[j].player1Socket == ev->data.fd)
                    {
                        found = j * 2;
                        break;
                    }
                    else if (th_data->games[j].player2Socket == ev->data.fd)
                    {
                        found = j * 2 + 1;
                        break;
                    }
                }
                if (found == -1)
                {
                    puts("Epoll has too many sockets.");
                    continue;
                }
                int game = found / 2;
                int player = found % 2;
                int buttons[2];
                int val = read(ev->data.fd, buttons, 2 * sizeof(int));
                printf("%d %d\n", buttons[0], buttons[1]);
                if (th_data->games[game].turn == NOTSTARTED)
                {
                    puts("Game did not start.");
                    int response = DIDNOTSTART;
                    int val = write(th_data->games[game].player1Socket, &response, sizeof(int));
                    val = write(th_data->games[game].player2Socket, &response, sizeof(int));
                    val = write(th_data->games[game].player1Socket, th_data->games[game].board, 8 * 8 * sizeof(int));
                    val = write(th_data->games[game].player2Socket, th_data->games[game].board, 8 * 8 * sizeof(int));
                    continue;
                }
                if (player + 1 != th_data->games[game].turn)
                {
                    puts("Not player's turn.");
                    int response = NOTYOURMOVE;
                    int val = write(th_data->games[game].player1Socket, &response, sizeof(int));
                    val = write(th_data->games[game].player2Socket, &response, sizeof(int));
                    val = write(th_data->games[game].player1Socket, th_data->games[game].board, 8 * 8 * sizeof(int));
                    val = write(th_data->games[game].player2Socket, th_data->games[game].board, 8 * 8 * sizeof(int));
                    continue;
                }
                th_data->games[game].board[buttons[0]][buttons[1]] = player + 1;
                int response = ACCEPTED;
                val = write(th_data->games[game].player1Socket, &response, 1 * sizeof(int));
                val = write(th_data->games[game].player1Socket, th_data->games[game].board, 8 * 8 * sizeof(int));
                val = write(th_data->games[game].player2Socket, &response, 1 * sizeof(int));
                val = write(th_data->games[game].player2Socket, th_data->games[game].board, 8 * 8 * sizeof(int));
                if (th_data->games[game].turn == PLAYER1)
                    th_data->games[game].turn = PLAYER2;
                else
                    th_data->games[game].turn = PLAYER1;
            }
            else if (ev->events & (EPOLLERR | EPOLLHUP))
            {
                puts("Client disconnected.");
            }
        }
    }

    pthread_exit(NULL);
}


int main(int argc, char* argv[])
{
    int server_socket_descriptor;
    int connection_socket_descriptor;
    int bind_result;
    int listen_result;
    char reuse_addr_val = 1;
    struct sockaddr_in server_address;

    memset(&server_address, 0, sizeof(struct sockaddr));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(atoi(argv[1]));

    server_socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_descriptor < 0)
    {
        perror("Socket");
        exit(EXIT_FAILURE);
    }
    setsockopt(server_socket_descriptor, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse_addr_val, sizeof(reuse_addr_val));

    bind_result = bind(server_socket_descriptor, (struct sockaddr*)&server_address, sizeof(struct sockaddr));
    if (bind_result < 0)
    {
        perror("Bind");
        exit(EXIT_FAILURE);
    }

    listen_result = listen(server_socket_descriptor, QUEUE_SIZE);
    if (listen_result < 0) {
        perror("Listen");
        exit(EXIT_FAILURE);
    }

    int epoll = epoll_create1(0);
    if (epoll == -1)
    {
        perror("Epoll create");
        exit(EXIT_FAILURE);
    }

    struct thread_data_t data;
    data.epoll = epoll;
    int create_result = 0;
    pthread_t thread1;
    create_result = pthread_create(&thread1, NULL, ThreadBehavior, &data);
    if (create_result){
        perror("Pthread create");
        exit(EXIT_FAILURE);
    }
    
    while(1)
    {
        connection_socket_descriptor = accept(server_socket_descriptor, NULL, NULL);
        if (connection_socket_descriptor < 0)
        {
            perror("Accept");
            exit(EXIT_FAILURE);
        }
        char lobbyName[80];
        int val = read(connection_socket_descriptor, &lobbyName, 80);
        if (val < 0)
        {
            perror("Lobby name.");
            exit(EXIT_FAILURE);
        }
        else if (val == 0)
        {
            continue;
        }

        int found = -1;
        for (int i = 0; i < data.size; i++)
        {
            if (!memcmp(data.games[i].lobbyName, lobbyName, 80))
            {
                if (data.games[i].player2Socket == 0)
                {
                    data.games[i].player2Socket = connection_socket_descriptor;
                    found = 1;
                    if (data.games[i].player1Socket != 0)
                        data.games[i].turn = PLAYER1;
                    data.games[i].board[3][3] = 1;
                    data.games[i].board[4][4] = 1;
                    data.games[i].board[3][4] = 2;
                    data.games[i].board[4][3] = 2;
                    break;
                }
                else if (data.games[i].player1Socket == 0)
                {
                    data.games[i].player1Socket = connection_socket_descriptor;
                    found = 1;
                    if (data.games[i].player2Socket != 0)
                        data.games[i].turn = PLAYER1;
                    data.games[i].board[3][3] = 1;
                    data.games[i].board[4][4] = 1;
                    data.games[i].board[3][4] = 2;
                    data.games[i].board[4][3] = 2;
                    break;
                }
                found = 0;
                break;
            }
        }
        if (found == -1)
        {
            if (data.size == MAX_GAMES)
            {
                int conn = FULLLOBBY;
                write(connection_socket_descriptor, &conn, sizeof(int));
                continue;
            }
            else
            {
                data.games[data.size].player1Socket = connection_socket_descriptor;
                memcpy(data.games[data.size++].lobbyName, lobbyName, 80);
            }
        }
        else if (found == 0)
        {
            puts("Full lobby.");
            int conn = FULLGAMES;
            write(connection_socket_descriptor, &conn, sizeof(int));
            continue;
        }
        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLET | EPOLLOUT | EPOLLHUP | EPOLLERR;
        ev.data.fd = connection_socket_descriptor;
        val = epoll_ctl(epoll, EPOLL_CTL_ADD, connection_socket_descriptor, &ev);
        if (val == -1)
        {
            perror("Adding to epoll.");
            exit(EXIT_FAILURE);
        }
        int resp = ACCEPTED;
        val = write(connection_socket_descriptor, &resp, sizeof(int));
        
    }

    close(server_socket_descriptor);
    return(0);
}
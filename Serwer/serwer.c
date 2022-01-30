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
#define TIMEOUT 20
#define PINGCHECK 3

enum RESPONSES {ACCEPTED = 0, FULLLOBBY = 1, FULLGAMES = 2, NOTYOURMOVE = 3, DIDNOTSTART = 4, ILLEGALMOVE = 5, FIELDTAKEN = 6, WAKEUP = 7, PLAYER1WON = 8, PLAYER2WON = 9, PING = 10, PINGACKNOWLEDGE = 11, DISCONNECT1 = 12, DISCONNECT2 = 13, MOVE = 14, DRAW = 15, STARTYOU = 16, STARTENEMY = 17};
enum TURN {NOTSTARTED, PLAYER1, PLAYER2, END};

struct game_t
{
    char lobbyName[80];
    int player1Socket;
    int lastAck1;
    int received1;
    int player2Socket;
    int lastAck2;
    int received2;
    int board[8][8];
    int freeFields;
    int turn;
};

void KillGame(struct game_t* game);

struct thread_data_t
{
    int epoll;
    struct game_t games[MAX_GAMES];
};

int VerifyBoard(int i, int j, int board[8][8], int player)
{
    int changed = 0;
    int ti = i;
    int tj = j;
    while (ti > -1)
    {
    ti--;
        if (board[ti][tj] == 0)
            break;
        if (board[ti][tj] == player + 1)
        {
            ti++;
            while (ti != i)
            {
                board[ti][tj] = player + 1;
                ti++;
                changed = 1;
            }
            break;
        }
    }
    ti = i;
    tj = j;
    while (ti < 8)
    {
    ti++;
        if (board[ti][tj] == 0)
            break;
        if (board[ti][tj] == player + 1)
        {
            ti--;
            while (ti != i)
            {
                board[ti][tj] = player + 1;
                ti--;
                changed = 1;
            }
            break;
        }
    }
    ti = i;
    tj = j;
    while (tj > -1)
    {
    tj--;
        if (board[ti][tj] == 0)
            break;
        if (board[ti][tj] == player + 1)
        {
            tj++;
            while (tj != j)
            {
                board[ti][tj] = player + 1;
                tj++;
                changed = 1;
            }
            break;
        }
    }
    ti = i;
    tj = j;
    while (ti < 8)
    {
    tj++;
        if (board[ti][tj] == 0)
            break;
        if (board[ti][tj] == player + 1)
        {
            tj--;
            while (tj != j)
            {
                board[ti][tj] = player + 1;
                tj--;
                changed = 1;
            }
            break;
        }
    }
    ti = i;
    tj = j;
    while (ti > -1 && tj > -1)
    {
    ti--;
    tj--;
        if (board[ti][tj] == 0)
            break;
        if (board[ti][tj] == player + 1)
        {
            ti++;
            tj++;
            while (tj != j && ti != i)
            {
                board[ti][tj] = player + 1;
                ti++;
                tj++;
                changed = 1;
            }
            break;
        }
    }
    ti = i;
    tj = j;
    while (ti > -1 && tj < 8)
    {
    ti--;
    tj++;
        if (board[ti][tj] == 0)
            break;
        if (board[ti][tj] == player + 1)
        {
            ti++;
            tj--;
            while (tj != j && ti != i)
            {
                board[ti][tj] = player + 1;
                ti++;
                tj--;
                changed = 1;
            }
            break;
        }
    }
    ti = i;
    tj = j;
    while (ti < 8 && tj < 8)
    {
    ti++;
    tj++;
        if (board[ti][tj] == 0)
            break;
        if (board[ti][tj] == player + 1)
        {
            ti--;
            tj--;
            while (tj != j && ti != i)
            {
                board[ti][tj] = player + 1;
                ti--;
                tj--;
                changed = 1;
            }
            break;
        }
    }
    ti = i;
    tj = j;
    while (ti < 8 && tj > -1)
    {
    ti++;
    tj--;
        if (board[ti][tj] == 0)
            break;
        if (board[ti][tj] == player + 1)
        {
            ti--;
            tj++;
            while (tj != j && ti != i)
            {
                board[ti][tj] = player + 1;
                ti--;
                tj++;
                changed = 1;
            }
            break;
        }
    }

    return changed;
}

void* CheckTimeouts(void* t_data)
{
    pthread_detach(pthread_self());
    struct thread_data_t *th_data = (struct thread_data_t*)t_data;
    for (;;)
    {
        for (int i = 0; i < MAX_GAMES; i++)
        {
            if (th_data->games[i].player1Socket != 0)
            {
                if (th_data->games[i].received1)
                {
                    puts("Ack received.");
                    int resp = PING;
                    int val = write(th_data->games[i].player1Socket, &resp, sizeof(int));
                    th_data->games[i].lastAck1 = time(NULL);
                    th_data->games[i].received1 = 0;
                    puts("Ping sent.");
                }
                else if (time(NULL) - th_data->games[i].lastAck1 > TIMEOUT)
                {
                    th_data->games[i].turn = END;
                    int response = DISCONNECT1;
                    int val = write(th_data->games[i].player1Socket, &response, sizeof(int));
                    val = write(th_data->games[i].player2Socket, &response, sizeof(int));
                    KillGame(&th_data->games[i]);
                }
            }
            if (th_data->games[i].player2Socket != 0)
            {
                if (th_data->games[i].received2)
                {
                    int resp = PING;
                    int val = write(th_data->games[i].player2Socket, &resp, sizeof(int));
                    th_data->games[i].lastAck2 = time(NULL);
                    th_data->games[i].received2 = 0;
                }
                else if (time(NULL) - th_data->games[i].lastAck2 > TIMEOUT)
                {
                    th_data->games[i].turn = END;
                    int response = DISCONNECT2;
                    int val = write(th_data->games[i].player1Socket, &response, sizeof(int));
                    val = write(th_data->games[i].player2Socket, &response, sizeof(int));
                    KillGame(&th_data->games[i]);
                }
            }
        }
        sleep(PINGCHECK);
    }
}

void KillGame(struct game_t* game)
{
    memset(game->lobbyName, 0, 80);
    game->player1Socket = 0;
    game->lastAck1 = INT32_MAX;
    game->player2Socket = 0;
    game->lastAck2 = INT32_MAX;
    memset(game->board, 0, 64);
    game->freeFields = 60;
    game->turn = NOTSTARTED;
}


void* EpollLoop(void* t_data)
{
    pthread_detach(pthread_self());
    struct thread_data_t *th_data = (struct thread_data_t*)t_data;
    struct epoll_event ev[MAX_EVENTS];
    for(;;) 
    {
        int val = epoll_wait(th_data->epoll, (struct epoll_event*)&ev, MAX_EVENTS, -1); 
        for (int i = 0; i < val; i++)
        {
            if (ev->events & EPOLLIN)
            {
                int found = -1;
                for (int j = 0; j < MAX_GAMES; j++)
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
                int clientResponse;
                int val = read(ev->data.fd, &clientResponse, sizeof(int));
                int game = found / 2;
                int player = found % 2;
                switch (clientResponse)
                {
                case MOVE:;
                    int buttons[2];
                    val = read(ev->data.fd, buttons, 2 * sizeof(int));
                    if (th_data->games[game].turn == NOTSTARTED)
                    {
                        puts("Game did not start.");
                        int response = DIDNOTSTART;
                        int val = write(ev->data.fd, &response, sizeof(int));
                        continue;
                    }
                    if (player + 1 != th_data->games[game].turn)
                    {
                        puts("Not player's turn.");
                        int response = NOTYOURMOVE;
                        int val = write(ev->data.fd, &response, sizeof(int));
                        continue;
                    }
                    if (th_data->games[game].board[buttons[0]][buttons[1]] != 0)
                    {
                        puts("Field taken.");
                        int response = FIELDTAKEN;
                        int val = write(ev->data.fd, &response, sizeof(int));
                        continue;
                    }
                    int board[8][8];
                    memcpy(board, th_data->games[game].board, 64 * sizeof(int));
                    int fieldsChanged = VerifyBoard(buttons[0], buttons[1], board, player);
                    if (!fieldsChanged)
                    {
                        puts("Illegal move.");
                        int response = ILLEGALMOVE;
                        int val = write(ev->data.fd, &response, sizeof(int));
                        continue;
                    }
                    memcpy(th_data->games[game].board, board, 64 * sizeof(int));
                    th_data->games[game].board[buttons[0]][buttons[1]] = player + 1;
                    puts("Move accepted.");
                    int response = ACCEPTED;
                    val = write(ev->data.fd , &response, 1 * sizeof(int));
                    val = write(ev->data.fd , th_data->games[game].board, 8 * 8 * sizeof(int));
                    response = WAKEUP;
                    if (ev->data.fd == th_data->games[game].player1Socket)
                    {
                        val = write(th_data->games[game].player2Socket, &response, 1 * sizeof(int));
                        val = write(th_data->games[game].player2Socket, th_data->games[game].board, 8 * 8 * sizeof(int));
                    }
                    else
                    {
                        val = write(th_data->games[game].player1Socket, &response, 1 * sizeof(int));
                        val = write(th_data->games[game].player1Socket, th_data->games[game].board, 8 * 8 * sizeof(int));
                    }
                    if (--th_data->games[game].freeFields == 0)
                    {
                        int blacks = 0, whites = 0;
                        for (int j = 0; j < 8; j++)
                        {
                            for (int k = 0; k < 8; k++)
                            {
                                if (th_data->games[game].board[j][k] == 1)
                                    blacks++;
                                else
                                    whites++;
                            }
                        }
                        int response;
                        if (blacks > whites)
                            response = PLAYER1WON;
                        else if (blacks == whites)
                            response = DRAW;
                        else
                            response = PLAYER2WON;
                        val = write(th_data->games[game].player1Socket, &response, sizeof(int));
                        val = write(th_data->games[game].player2Socket, &response, sizeof(int));
                        KillGame(&th_data->games[game]);
                    }
                    if (th_data->games[game].turn == PLAYER1)
                        th_data->games[game].turn = PLAYER2;
                    else
                        th_data->games[game].turn = PLAYER1;
                    break;
                case PINGACKNOWLEDGE:
                    if (ev->data.fd == th_data->games[game].player1Socket)
                    {
                        th_data->games[game].received1 = 1;
                    }
                    else
                    {
                        th_data->games[game].received2 = 1;
                    }
                    break;
                }
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
    signal(SIGPIPE, SIG_IGN);
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
    if (listen_result < 0) 
    {
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
    memset(&data, 0, sizeof(struct thread_data_t));
    data.epoll = epoll;
    int create_result = 0;
    pthread_t thread1, thread2;
    create_result = pthread_create(&thread1, NULL, EpollLoop, &data);
    if (create_result)
    {
        perror("Pthread create");
        exit(EXIT_FAILURE);
    }
    create_result = pthread_create(&thread2, NULL, CheckTimeouts, &data);
    if (create_result)
    {
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

        int foundExist = -1;
        for (int i = 0; i < MAX_GAMES; i++)
        {
            if (!memcmp(data.games[i].lobbyName, lobbyName, 80))
            {
                if (data.games[i].player2Socket == 0)
                {
                    data.games[i].player2Socket = connection_socket_descriptor;
                    foundExist = i + 1;
                    if (data.games[i].player1Socket != 0)
                        data.games[i].turn = PLAYER1;
                    data.games[i].board[3][3] = 1;
                    data.games[i].board[4][4] = 1;
                    data.games[i].board[3][4] = 2;
                    data.games[i].board[4][3] = 2;
                    data.games[i].freeFields = 60;
                    int resp = STARTYOU;
                    val = write(data.games[i].player1Socket, &resp, sizeof(int));
                    break;
                }
                foundExist = 0;
                break;
            }
        }
        int foundFree;
        if (foundExist == -1)
        {
            for (foundFree = 0; foundFree < MAX_GAMES; foundFree++)
                if (!strlen(data.games[foundFree].lobbyName))
                    break;
            if (foundFree == MAX_GAMES)
            {
                int conn = FULLLOBBY;
                write(connection_socket_descriptor, &conn, sizeof(int));
                continue;
            }
            else
            {
                data.games[foundFree].player1Socket = connection_socket_descriptor;
                memcpy(data.games[foundFree].lobbyName, lobbyName, 80);
            }
        }
        else if (foundExist == 0)
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
        if (foundExist != -1)
        {
            int resp = STARTENEMY;
            val = write(connection_socket_descriptor, &resp, sizeof(int));
            data.games[foundExist - 1].received2 = 1;
            data.games[foundExist - 1].lastAck2 = time(NULL);
        }
        else
        {
            printf("%d\n", foundFree);
            data.games[foundFree].received1 = 1;
            data.games[foundFree].lastAck1 = time(NULL);
        }
    }

    close(server_socket_descriptor);
    return(0);
}
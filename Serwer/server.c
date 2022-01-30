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
#include "boardfunctions.h"

#define QUEUE_SIZE 5
#define MAX_GAMES 16
#define MAX_EVENTS 10

/*
 *  Typ wyliczeniowy przechowujacy informacje o typie komunikatu.
 */
enum RESPONSES {ACCEPTED = 0, FULLLOBBY = 1, FULLGAMES = 2, NOTYOURMOVE = 3, DIDNOTSTART = 4, ILLEGALMOVE = 5, FIELDTAKEN = 6, WAKEUP = 7, PLAYER1WON = 8, PLAYER2WON = 9, DISCONNECT1 = 12, DISCONNECT2 = 13, MOVE = 14, DRAW = 15, STARTYOU = 16, STARTENEMY = 17};

/*
 *  Struktura przekazywana do watku. Zawiera uchwyt na instancje epoll i informacje o wszystkich grach.
 */
struct thread_data_t
{
    int epoll;
    struct game_t games[MAX_GAMES];
    pthread_mutex_t* mut;
};


/*
 *  Funkcja szukajaca gracza w grach o podanym gniezdzie.
 */
int FindGame(struct game_t games[MAX_GAMES], int fd)
{
    for (int i = 0; i < MAX_GAMES; i++)
    {
        if (games[i].player1Socket == fd)
        {
            return i * 2;
        }
        else if (games[i].player2Socket == fd)
        {
            return i * 2 + 1;
        }
    }
    return -1;
}


void* EventLoop(void* t_data)
{
    pthread_detach(pthread_self());
    struct thread_data_t *th_data = (struct thread_data_t*)t_data;
    struct epoll_event ev[MAX_EVENTS];
    for(;;) 
    {
        int numberOfEvents = epoll_wait(th_data->epoll, (struct epoll_event*)&ev, MAX_EVENTS, -1); 
        for (int i = 0; i < numberOfEvents; i++)
        {
            pthread_mutex_lock(th_data->mut);
            if (ev->events & EPOLLIN)
            {
                int found = FindGame(th_data->games, ev->data.fd);
                int game = found / 2;
                int player = found % 2;
                int clientResponse;
                int val = read(ev->data.fd, &clientResponse, sizeof(int));
                if (val == -1)
                {
                    perror("Reading client's message type");
                    exit(EXIT_FAILURE);
                }
                else if (val == 0)
                {
                    int response;
                    if (player == 0)
                        response = PLAYER1;
                    else
                        response = PLAYER2;
                    if (write(th_data->games[game].player1Socket, &response, sizeof(int)) == -1)
                        perror("Telling player that player disconnected");
                    if (write(th_data->games[game].player2Socket, &response, sizeof(int)) == -1)
                        perror("Telling player that player disconnected");
                    FreeGame(&th_data->games[game]);
                    pthread_mutex_unlock(th_data->mut);
                    continue;
                }
                
                switch (clientResponse)
                {
                case MOVE:;
                    int buttons[2];
                    val = read(ev->data.fd, buttons, 2 * sizeof(int));
                    if (val == -1)
                    {
                        perror("Reading what button client pressed");
                        exit(EXIT_FAILURE);
                    }
                    else if (val == 0)
                    {
                        int response;
                        if (player == 0)
                            response = PLAYER1;
                        else
                            response = PLAYER2;
                        if (write(th_data->games[game].player1Socket, &response, sizeof(int)) == -1)
                            perror("Telling player that player disconnected");
                        if (write(th_data->games[game].player2Socket, &response, sizeof(int)) == -1)
                            perror("Telling player that player disconnected");
                        FreeGame(&th_data->games[game]);
                        pthread_mutex_unlock(th_data->mut);
                        continue;
                    }
                    if (th_data->games[game].turn == NOTSTARTED)
                    {
                        int response = DIDNOTSTART;
                        if (write(ev->data.fd, &response, sizeof(int)) == -1)
                        {
                            perror("Telling player that the game hasn't started yet");
                            exit(EXIT_FAILURE);
                        }
                        pthread_mutex_unlock(th_data->mut);
                        continue;
                    }
                    if (player + 1 != th_data->games[game].turn)
                    {
                        int response = NOTYOURMOVE;
                        if (write(ev->data.fd, &response, sizeof(int)) == -1)
                        {
                            perror("Telling player that it's not his time to move");
                            exit(EXIT_FAILURE);
                        }
                        pthread_mutex_unlock(th_data->mut);
                        continue;
                    }
                    if (th_data->games[game].board[buttons[0]][buttons[1]] != 0)
                    {
                        int response = FIELDTAKEN;
                        if (write(ev->data.fd, &response, sizeof(int)) == -1)
                        {
                            perror("Telling player that he tried to take taken field");
                            exit(EXIT_FAILURE);
                        }
                        pthread_mutex_unlock(th_data->mut);
                        continue;
                    }
                    int board[8][8];
                    memcpy(board, th_data->games[game].board, 64 * sizeof(int));
                    int fieldsChanged = VerifyBoard(buttons[0], buttons[1], board, player);
                    if (!fieldsChanged)
                    {
                        int response = ILLEGALMOVE;
                        if (write(ev->data.fd, &response, sizeof(int)) == -1)
                        {
                            perror("Telling player that his move is not valid");
                            exit(EXIT_FAILURE);
                        }
                        pthread_mutex_unlock(th_data->mut);
                        continue;
                    }
                    memcpy(th_data->games[game].board, board, 64 * sizeof(int));
                    th_data->games[game].board[buttons[0]][buttons[1]] = player + 1;
                    int response = ACCEPTED;
                    if (write(ev->data.fd , &response, 1 * sizeof(int)) == -1)
                    {
                            perror("Telling player that move was accepted");
                            exit(EXIT_FAILURE);
                    }
                    if (write(ev->data.fd , th_data->games[game].board, 8 * 8 * sizeof(int)) == -1)
                    {
                            perror("Telling player that move was accepted");
                            exit(EXIT_FAILURE);
                    }
                    response = WAKEUP;
                    if (ev->data.fd == th_data->games[game].player1Socket)
                    {
                        if (write(th_data->games[game].player2Socket, &response, 1 * sizeof(int)) == -1)
                        {
                            perror("Telling player 2 to make a move");
                            exit(EXIT_FAILURE);
                        }
                        if (write(th_data->games[game].player2Socket, th_data->games[game].board, 8 * 8 * sizeof(int)) == -1)
                        {
                            perror("Telling player 2 to make a move");
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        if (write(th_data->games[game].player1Socket, &response, 1 * sizeof(int)) == -1)
                        {
                            perror("Telling player 1 to make a move");
                            exit(EXIT_FAILURE);
                        }
                        if (write(th_data->games[game].player1Socket, th_data->games[game].board, 8 * 8 * sizeof(int)) == -1)
                        {
                            perror("Telling player 1 to make a move");
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (--th_data->games[game].freeFields == 0 || !CheckForMoves(th_data->games[game].board, player == 0 ? 1 : 0))
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
                        if (write(th_data->games[game].player1Socket, &response, sizeof(int)) == -1)
                            perror("Sending results to player 1");
                        if (write(th_data->games[game].player2Socket, &response, sizeof(int)) == -1)
                            perror("Sending results to player 2");
                        FreeGame(&th_data->games[game]);
                        break;
                    }
                    if (th_data->games[game].turn == PLAYER1)
                        th_data->games[game].turn = PLAYER2;
                    else
                        th_data->games[game].turn = PLAYER1;
                    break;
                }
            }
            else if (ev->events & (EPOLLERR | EPOLLHUP))
            {
                int found = FindGame(th_data->games, ev->data.fd);
                int game = found / 2;
                int player = found % 2;
                int response;
                if (player == 0)
                    response = PLAYER1;
                else
                    response = PLAYER2;
                if (write(th_data->games[game].player1Socket, &response, sizeof(int)) == -1)
                    puts("Telling player that player disconnected");
                if (write(th_data->games[game].player2Socket, &response, sizeof(int)) == -1)
                    puts("Telling player that player disconnected");
                FreeGame(&th_data->games[game]);
            }
            pthread_mutex_unlock(th_data->mut);
        }
    }

    pthread_exit(NULL);
}


int main(int argc, char* argv[])
{
    signal(SIGPIPE, SIG_IGN);
    char reuse_addr_val = 1;
    struct sockaddr_in server_address;

    memset(&server_address, 0, sizeof(struct sockaddr));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(atoi(argv[1]));

    int server_socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_descriptor == -1)
    {
        perror("Creating socket");
        exit(EXIT_FAILURE);
    }
    setsockopt(server_socket_descriptor, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse_addr_val, sizeof(char));

    if (bind(server_socket_descriptor, (struct sockaddr*)&server_address, sizeof(struct sockaddr)) == -1)
    {
        perror("Bind function");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket_descriptor, QUEUE_SIZE) == -1) 
    {
        perror("Listen function");
        exit(EXIT_FAILURE);
    }

    int epoll = epoll_create1(0);
    if (epoll == -1)
    {
        perror("Creating epoll");
        exit(EXIT_FAILURE);
    }

    struct thread_data_t data;
    memset(&data, 0, sizeof(struct thread_data_t));
    data.epoll = epoll;
    pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
    data.mut = &mut;
    pthread_t thread1;
    if (pthread_create(&thread1, NULL, EventLoop, &data) != 0)
    {
        perror("Creating thread");
        exit(EXIT_FAILURE);
    }
    for (;;)
    {
        int connection_socket_descriptor = accept(server_socket_descriptor, NULL, NULL);
        if (connection_socket_descriptor == -1)
        {
            perror("Accepting new connection");
            exit(EXIT_FAILURE);
        }
        char lobbyName[80];
        int val = read(connection_socket_descriptor, &lobbyName, 80);
        if (val == -1)
        {
            perror("Reading lobby name");
            exit(EXIT_FAILURE);
        }
        else if (val == 0)
        {
            continue;
        }

        int foundExist = -1;
        pthread_mutex_lock(&mut);
        for (int i = 0; i < MAX_GAMES; i++)
        {
            if (!memcmp(data.games[i].lobbyName, lobbyName, 80))
            {
                if (data.games[i].player2Socket == 0)
                {
                    data.games[i].player2Socket = connection_socket_descriptor;
                    foundExist = i + 1;
                    InitBoard(&data.games[i]);
                    int resp = STARTYOU;
                    if (write(data.games[i].player1Socket, &resp, sizeof(int)) == -1)
                    {
                        perror("Telling player 1 to make his first move");
                        exit(EXIT_FAILURE);
                    }
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
                int conn = FULLGAMES;
                if (write(connection_socket_descriptor, &conn, sizeof(int)) == -1)
                    perror("Telling player 1 that all lobbies are taken");
                else
                    close(connection_socket_descriptor);
                pthread_mutex_unlock(&mut);
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
            int conn = FULLLOBBY;
            if (write(connection_socket_descriptor, &conn, sizeof(int)) == -1)
                perror("Telling player that the lobby is full");
            else
                close(connection_socket_descriptor);
            pthread_mutex_unlock(&mut);
            continue;
        }
        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLET | EPOLLHUP | EPOLLERR;
        ev.data.fd = connection_socket_descriptor;
        val = epoll_ctl(epoll, EPOLL_CTL_ADD, connection_socket_descriptor, &ev);
        if (val == -1)
        {
            perror("Adding socket to epoll");
            exit(EXIT_FAILURE);
        }
        int resp = ACCEPTED;
        if (write(connection_socket_descriptor, &resp, sizeof(int)) == -1)
        {
            perror("Telling player 1 that he joined successfully");
            exit(EXIT_FAILURE);
        }
        if (foundExist != -1)
        {
            int resp = STARTENEMY;
            if (write(connection_socket_descriptor, &resp, sizeof(int)) == -1)
            {
                perror("Telling player 2 that he joined successfully");
                exit(EXIT_FAILURE);
            }
        }
        pthread_mutex_unlock(&mut);
    }

    return 0;
}
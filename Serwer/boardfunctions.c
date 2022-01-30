#include "boardfunctions.h"

int VerifyBoard(int i, int j, int board[8][8], int player)
{
    int changed = 0;
    int ti = i;
    int tj = j;
    while (--ti > -1)
    {
        if (board[ti][tj] == 0)
            break;
        else if (board[ti][tj] == player + 1)
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
    while (++ti < 8)
    {
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
    while (--tj > -1)
    {
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
    while (++tj < 8)
    {
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
    while (--ti > -1 && --tj > -1)
    {
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
    while (--ti > -1 && ++tj < 8)
    {
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
    while (++ti < 8 && ++tj < 8)
    {
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
    while (++ti < 8 && --tj > -1)
    {
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

int CheckForMoves(int board[8][8], int player)
{
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 8; j++)
        {
            if (board[i][j] == 0)
            {
                int tmp[8][8];
                memcpy(tmp, board, 64 * sizeof(int));
                if (VerifyBoard(i, j, tmp, player))
                    return 1;
            }
        }
    return 0;
}


void InitBoard(struct game_t* game)
{
    memset(game->board, 0, 64 * sizeof(int));
    game->board[3][3] = 1;
    game->board[4][4] = 1;
    game->board[3][4] = 2;
    game->board[4][3] = 2;
    game->turn = PLAYER1;
    game->freeFields = 60;
}



void FreeGame(struct game_t* game)
{
    close(game->player1Socket);
    close(game->player2Socket);
    memset(game->lobbyName, 0, 80 * sizeof(char));
    game->player1Socket = 0;
    game->player2Socket = 0;
    memset(game->board, 0, 64 * sizeof(int));
    game->freeFields = 60;
    game->turn = NOTSTARTED;
}
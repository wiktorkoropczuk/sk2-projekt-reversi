#pragma once
#include <string.h>
#include <unistd.h>

/*
 *  Typ wyliczeniowy przechowujacy informacje o tym, kto moze wykonac ruch.
 */
enum TURN {NOTSTARTED = 0, PLAYER1 = 1, PLAYER2 = 2};

/*
 *  Struktura przechowujaca informacje o danej rozgrywce.
 */
struct game_t
{
    char lobbyName[80];
    int player1Socket;
    int player2Socket;
    int board[8][8];
    int freeFields;
    int turn;
};

/*
 *  Funkcja sprawdzajaca, czy mozna zajac pole o pozycji (i, j).
 */
int VerifyBoard(int i, int j, int board[8][8], int player);

/*
 *  Funkcja sprawdzajaca, czy gracz moze wykonac przynajmniej jeden ruch.
 */
int CheckForMoves(int board[8][8], int player);

/*
 *  Funkcja przygotowujaca strukture game_t do gry. Wywolywana, gdy
 *  dwoch graczy dolaczy do gry.
 */
void InitBoard(struct game_t* game);

/*
 *  Funkcja resetujaca strukture game_t.
 */
void FreeGame(struct game_t* game);

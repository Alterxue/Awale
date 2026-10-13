#ifndef AWALE_H
#define AWALE_H

#include <stdbool.h>

typedef struct Cell {
struct Cell* prev;
struct Cell* next;
int playerId;
int ballsNb;
} Cell;

typedef struct Board {
Cell* headPlayerOne;
Cell* headPlayerTwo;
int scorePlayerOne;
int scorePlayerTwo;
int rowSize;
int totalBallNumber;
} Board;

Board* initBoard(void);

Cell* getPickedCell(Cell* head, int cellNb);
Cell* distributeBalls(Cell* initialCell);
int claimBalls(Cell* fromCell, int playerId);
Cell* getPlayerHead(Board* board, int joueurId);

bool canPlayerPlay(Board* board, int joueurId);
int hasPlayerWon(Board* board);

bool isMoveValid(Board* board, int playerId, int cellNb, Cell* playerHead);
int play(Board* board, int playerId, int cellNb);

char* printBoard(Board* board, int playerId, char* playerTurnName);

#endif 

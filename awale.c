#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>

// Condition de fin
// x famine d'un joueur
// x score > à la moitié
// - 5 coups où personne mange rien

// Condition coup invalide :
// - mettre famine à l'autre joueur alors qu'on peut l'éviter
// - choisir une cellule sans boules

// #define INITIAL_BALLS_NUMBER_IN_CELL 2;
// #define ROW_SIZE 2;

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

Board* initBoard() {
	// CONST
	int INITIAL_BALLS_NUMBER = 4;
	int ROW_SIZE = 6;

	// Init Board
	Board *board = malloc(sizeof(Board));
    board->headPlayerOne = NULL;
	board->headPlayerTwo = NULL;
	board->rowSize = ROW_SIZE;
	board->scorePlayerOne = 0;
	board->scorePlayerTwo = 0;
	board->totalBallNumber = INITIAL_BALLS_NUMBER*12;

	// Init First Cell
	Cell *head = malloc(sizeof(Cell));
	head->playerId = 1;
	head->ballsNb = INITIAL_BALLS_NUMBER;
	head->prev = NULL;
	head->next = NULL;

	board->headPlayerOne = head;

	Cell *curr = head;
	int playerId = 1;
	for (int i = 1; i < board->rowSize * 2; i++) {
		Cell* newCell = malloc(sizeof(Cell));
		
		if (playerId == 1 && i >=  board->rowSize) {
			playerId = 2;
		}
			
		newCell->playerId = playerId;
		newCell->ballsNb = INITIAL_BALLS_NUMBER;
		newCell->next = NULL;

		// chainage
		newCell->prev = curr;
		curr->next = newCell;

		if (playerId == 2 && i == board->rowSize) {
			board->headPlayerTwo = newCell;
		}

		curr = newCell;
	}

	curr->next = head;
	head->prev = curr;
	
	return board;
}

Cell* getPickedCell(Cell* head, int cellNb) {
	Cell* cell = head;
	for (int i = 1; i < cellNb; i++) {
		cell = cell->next;
	}
	return cell;
}

// return last cell where a ball was added
Cell* distributeBalls(Cell* initialCell) {
	int ballsToDistribute = initialCell->ballsNb;
	initialCell->ballsNb = 0;

	Cell* curr = initialCell->next;
	
	for (int i = 0; i < ballsToDistribute; i++) {
		++curr->ballsNb;
		if (i != ballsToDistribute - 1) {
			curr = curr->next;
		}
	}
	return curr;
}	

int claimBalls(Cell* fromCell, int playerId) {
	int claimedBalls = 0;
	Cell* curr = fromCell;
	while (curr->playerId != playerId) {
		if (curr->ballsNb == 2 || curr->ballsNb == 3)  {
			claimedBalls += curr->ballsNb;
			curr->ballsNb = 0;
		} else {
			return claimedBalls;
		}
		curr = curr->prev;
	}	
	return claimedBalls;
}

Cell* getPlayerHead(Board* board, int joueurId) {
	if (joueurId == 1) {
		return board->headPlayerOne;
	}
	return board->headPlayerTwo;
}

bool canPlayerPlay(Board* board, int joueurId) {
	Cell* curr = getPlayerHead(board, joueurId);
	while (curr->playerId == joueurId) {
		if (curr->ballsNb > 0) {
			return true;
		}
		curr = curr->next;
	}
	return false;
}

int hasPlayerWon(Board* board) {
	if (board->scorePlayerOne > board->totalBallNumber / 2) {
		return 1;
	}
	if (board->scorePlayerTwo > board->totalBallNumber / 2) {
		return 2;
	}
	return 0;
}

bool isMoveValid(Board* board, int playerId, int cellNb, Cell* playerHead) {
	int otherPlayerId = 3 - playerId;
	if (canPlayerPlay(board, otherPlayerId) == false) {
		Cell* curr = playerHead;
		for (int i = 0; i < 6; i++) {
			if (curr->ballsNb - (board->rowSize - i) >= 0) {
				if (cellNb == i + 1) {
					return true;
				}
			}
			curr = curr->next;
		}
		return false;
	}
	return true;
}

// If return 1, playerOne Win, if return 2, playerTwo win, else, game continue
int play(Board* board, int playerId, int cellNb) {
	if (playerId > 2 || playerId < 1) return -1;
	if (cellNb < 1 || cellNb > board->rowSize) return -1;

	Cell* playerHeadCell = getPlayerHead(board, playerId);
	Cell *initialCell = getPickedCell(playerHeadCell, cellNb);

	if (initialCell->ballsNb == 0) return -1;
	if (isMoveValid(board, playerId, cellNb, playerHeadCell) == false) return -1;
	
	Cell* lastCell = distributeBalls(initialCell);
	int scoreToAdd = claimBalls(lastCell, playerId);
	
	if (playerId == 1) {
		board->scorePlayerOne += scoreToAdd;
	} else {
		board->scorePlayerTwo += scoreToAdd;
	}

	// Check victory
	int otherPlayerId = 3 - playerId;
	if (canPlayerPlay(board, otherPlayerId) == false) {
		return playerId;
	}
	return hasPlayerWon(board); 
}

char* printBoard(Board* board, int playerId) {
	if (playerId < 1 && playerId > 2) return NULL;
	
	char* row = malloc(sizeof(char)*30);
	sprintf(row, "  - - - - - - - - - - - -\n");
	
	char* display = malloc(sizeof(char)*500);
	int offset = 0;	

	Cell* firstPlayerCell = getPlayerHead(board, playerId);

	// Score row
	offset += sprintf(display + offset, "Score : \n");
	offset += sprintf(display + offset, " -  Player 1 : %d\n", board->scorePlayerOne);
	offset += sprintf(display + offset, " -  Player 2 : %d\n\n", board->scorePlayerTwo);

	offset += sprintf(display + offset, "%s", row);
	
	// Opponent row
	Cell* curr = firstPlayerCell->prev;
	for (int i = 0; i < board->rowSize; i++) {
                offset += sprintf(display + offset, " | %d", curr->ballsNb);
                curr = curr->prev;
		if (i == board->rowSize - 1) {
			offset += sprintf(display + offset, " |\n");
		}
        }
	
	offset += sprintf(display + offset, "%s", row);

	// Player row
	curr = firstPlayerCell;
	for (int i = 0; i < board->rowSize; i++) {
		offset += sprintf(display + offset, " | %d", curr->ballsNb);
		curr = curr->next;
		if (i == board->rowSize - 1) {
			offset += sprintf(display + offset, " |\n");
		}
	}

	sprintf(display + offset, "%s\n", row);
	return display;
}

void testFamineConfig(Board* board) {
	Cell *curr = board->headPlayerTwo;
	while (curr->playerId == 2) {
		curr->ballsNb = 0;
		curr = curr->next;
	}
	while (curr->playerId == 1) {
		curr->ballsNb = 3;
		curr = curr->next;
	}
}
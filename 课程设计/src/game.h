#ifndef GOBANG_GAME_H
#define GOBANG_GAME_H

#include <stdbool.h>

#define BOARD_SIZE 15
#define BOARD_CELLS (BOARD_SIZE * BOARD_SIZE)

typedef enum { EMPTY, BLACK, WHITE } Stone;

typedef struct {
    int row;
    int col;
} Move;

typedef struct {
    Stone cells[BOARD_SIZE][BOARD_SIZE];
    Move history[BOARD_CELLS];
    int count;
    Stone turn;
    Stone player;
    Stone winner;
    bool draw;
    Move win_start;
    Move win_end;
} Game;

Stone game_other(Stone stone);
bool game_in_bounds(Move move);
void game_reset(Game *game, Stone player);
bool game_can_play(const Game *game, Move move);
bool game_play(Game *game, Move move);
bool game_can_undo(const Game *game);
bool game_undo(Game *game);
bool game_over(const Game *game);

/* Test a stone already at move or a hypothetical stone at an empty move. */
bool game_winning_line(const Game *game, Move move, Stone stone,
                       Move *start, Move *end);

#endif

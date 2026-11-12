#include "game.h"

Stone game_other(Stone stone)
{
    switch (stone) {
    case BLACK: return WHITE;
    case WHITE: return BLACK;
    default: return EMPTY;
    }
}

bool game_in_bounds(Move move)
{
    return move.row >= 0 && move.row < BOARD_SIZE &&
           move.col >= 0 && move.col < BOARD_SIZE;
}

void game_reset(Game *game, Stone player)
{
    *game = (Game){
        .turn = BLACK,
        .player = player,
        .win_start = {-1, -1},
        .win_end = {-1, -1},
    };
}

bool game_over(const Game *game)
{
    return game->winner != EMPTY || game->draw;
}

bool game_can_play(const Game *game, Move move)
{
    return !game_over(game) && game_in_bounds(move) &&
           game->cells[move.row][move.col] == EMPTY;
}

static Move line_end(const Game *game, Move move, Stone stone, int dr, int dc)
{
    Move next = {move.row + dr, move.col + dc};
    while (game_in_bounds(next) && game->cells[next.row][next.col] == stone) {
        move = next;
        next.row += dr;
        next.col += dc;
    }
    return move;
}

bool game_winning_line(const Game *game, Move move, Stone stone,
                       Move *start, Move *end)
{
    static const Move directions[] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};

    if (!game_in_bounds(move) || (stone != BLACK && stone != WHITE)) {
        return false;
    }
    const Stone cell = game->cells[move.row][move.col];
    if (cell != EMPTY && cell != stone) {
        return false;
    }

    for (int i = 0; i < 4; ++i) {
        const int dr = directions[i].row;
        const int dc = directions[i].col;
        const Move first = line_end(game, move, stone, -dr, -dc);
        const Move last = line_end(game, move, stone, dr, dc);
        const int distance = dr == 0 ? last.col - first.col : last.row - first.row;
        if (distance >= 4) {
            if (start) *start = first;
            if (end) *end = last;
            return true;
        }
    }
    return false;
}

bool game_play(Game *game, Move move)
{
    if (!game_can_play(game, move)) {
        return false;
    }

    game->cells[move.row][move.col] = game->turn;
    game->history[game->count++] = move;
    if (game_winning_line(game, move, game->turn, &game->win_start, &game->win_end)) {
        game->winner = game->turn;
    } else if (game->count == BOARD_CELLS) {
        game->draw = true;
    } else {
        game->turn = game_other(game->turn);
    }
    return true;
}

bool game_can_undo(const Game *game)
{
    const int opening_moves = game->player == WHITE ? 1 : 0;
    return !game_over(game) && game->count > opening_moves;
}

bool game_undo(Game *game)
{
    if (!game_can_undo(game)) {
        return false;
    }

    /* If the AI already replied, remove its move along with the player's. */
    const int remove = game->turn == game->player ? 2 : 1;
    for (int i = 0; i < remove; ++i) {
        const Move last = game->history[--game->count];
        game->cells[last.row][last.col] = EMPTY;
        game->history[game->count] = (Move){0};
    }
    game->turn = game->player;
    return true;
}

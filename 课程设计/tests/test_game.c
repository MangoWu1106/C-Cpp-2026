#include "game.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(expression)                                                       \
    do {                                                                        \
        if (!(expression)) {                                                    \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expression);      \
            exit(EXIT_FAILURE);                                                 \
        }                                                                       \
    } while (0)

static bool same_move(Move a, Move b)
{
    return a.row == b.row && a.col == b.col;
}

static void test_illegal_moves(void)
{
    Game game;
    game_reset(&game, WHITE);
    CHECK(game.player == WHITE);
    CHECK(game.turn == BLACK);
    CHECK(game_play(&game, (Move){3, 4}));
    CHECK(game.cells[3][4] == BLACK);
    CHECK(game.turn == WHITE);

    unsigned char snapshot[sizeof game];
    memcpy(snapshot, &game, sizeof game);
    CHECK(!game_play(&game, (Move){3, 4}));
    CHECK(!game_play(&game, (Move){-1, 4}));
    CHECK(!game_play(&game, (Move){3, BOARD_SIZE}));
    CHECK(!game_winning_line(&game, (Move){3, 4}, WHITE, NULL, NULL));
    CHECK(!game_winning_line(&game, (Move){3, 4}, EMPTY, NULL, NULL));
    CHECK(memcmp(snapshot, &game, sizeof game) == 0);
}

static void test_winning_directions(void)
{
    const Move starts[] = {{0, 10}, {10, 14}, {10, 10}, {10, 4}};
    const Move steps[] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};

    for (int i = 0; i < 4; ++i) {
        Game game;
        game_reset(&game, BLACK);
        Move next = starts[i];
        for (int j = 0; j < 4; ++j) {
            CHECK(game_play(&game, next));
            CHECK(game_play(&game, (Move){1, 2 * j}));
            CHECK(!game_over(&game));
            next.row += steps[i].row;
            next.col += steps[i].col;
        }
        CHECK(game_play(&game, next));
        CHECK(game_over(&game));
        CHECK(game.winner == BLACK && !game.draw);
        CHECK(same_move(game.win_start, starts[i]));
        CHECK(same_move(game.win_end, next));
        CHECK(!game_play(&game, (Move){8, 8}));
        CHECK(!game_undo(&game));
    }
}

static void test_overline(Stone stone)
{
    const int order[] = {0, 1, 3, 4, 5, 2};
    Game game;
    game_reset(&game, BLACK);
    int fillers = 0;
    for (int i = 0; i < 5; ++i) {
        if (game.turn != stone) {
            CHECK(game_play(&game, (Move){11, 2 * fillers}));
            ++fillers;
        }
        CHECK(game_play(&game, (Move){2, 2 + order[i]}));
        CHECK(!game_over(&game));
    }
    CHECK(game_play(&game, (Move){11, 2 * fillers}));

    const Move finish = {2, 2 + order[5]};
    Move start, end;
    unsigned char snapshot[sizeof game];
    memcpy(snapshot, &game, sizeof game);
    CHECK(game_winning_line(&game, finish, stone, &start, &end));
    CHECK(memcmp(snapshot, &game, sizeof game) == 0);
    CHECK(same_move(start, (Move){2, 2}));
    CHECK(same_move(end, (Move){2, 7}));
    CHECK(game_play(&game, finish));
    CHECK(game.winner == stone);
    CHECK(same_move(game.win_start, start) && same_move(game.win_end, end));
}

static void test_full_board(bool win_on_last_move)
{
    const Move finish = {0, 3};
    Move moves[3][BOARD_CELLS];
    int counts[3] = {0};
    int next[3] = {0};
    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            const Move move = {row, col};
            Stone stone = (row + col / 2) % 2 == 0 ? BLACK : WHITE;
            if (win_on_last_move) {
                /* Keep the color counts and leave only the six at row zero. */
                if (same_move(move, finish)) continue;
                if (row == 0 && col == 2) stone = BLACK;
                if ((row == 12 || row == 14) && col == 12) stone = WHITE;
            }
            moves[stone][counts[stone]++] = move;
        }
    }
    if (win_on_last_move) {
        moves[BLACK][counts[BLACK]++] = finish;
    }
    CHECK(counts[BLACK] == 113 && counts[WHITE] == 112);

    Game game;
    game_reset(&game, BLACK);
    for (int i = 0; i < BOARD_CELLS; ++i) {
        CHECK(!game_over(&game));
        const Stone turn = game.turn;
        CHECK(game_play(&game, moves[turn][next[turn]++]));
    }
    CHECK(game_over(&game));
    CHECK(game.count == BOARD_CELLS);
    CHECK(game.winner == (win_on_last_move ? BLACK : EMPTY));
    CHECK(game.draw == !win_on_last_move);
    CHECK(!game_undo(&game));
}

static void test_undo(Stone player)
{
    const Move opening = {7, 7};
    const Move decision = {8, 8};
    const Move reply = {8, 7};
    const int opening_moves = player == WHITE ? 1 : 0;

    Game game;
    game_reset(&game, player);
    CHECK(!game_undo(&game));
    if (opening_moves) {
        CHECK(game_play(&game, opening));
        CHECK(!game_undo(&game));
        CHECK(game.cells[7][7] == BLACK);
    }

    CHECK(game_play(&game, decision));
    CHECK(game.turn == game_other(player));
    CHECK(game_can_undo(&game));
    CHECK(game_undo(&game));
    CHECK(game.turn == player);
    CHECK(game.count == opening_moves);
    CHECK(game.cells[8][8] == EMPTY);

    CHECK(game_play(&game, decision));
    CHECK(game_play(&game, reply));
    CHECK(game.turn == player);
    CHECK(game_can_undo(&game));
    CHECK(game_undo(&game));
    CHECK(game.turn == player);
    CHECK(game.count == opening_moves);
    CHECK(game.cells[8][8] == EMPTY);
    CHECK(game.cells[8][7] == EMPTY);
    CHECK(game.cells[7][7] == (opening_moves ? BLACK : EMPTY));
    CHECK(!game_undo(&game));
}

int main(void)
{
    test_illegal_moves();
    test_winning_directions();
    test_overline(BLACK);
    test_overline(WHITE);
    test_full_board(false);
    test_full_board(true);
    test_undo(BLACK);
    test_undo(WHITE);
    puts("Gobang rules checks passed.");
    return EXIT_SUCCESS;
}

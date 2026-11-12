/* Include the implementation to compare the same candidate tree in both searches. */
#include "../src/ai.c"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>

static Game replay(const Move *moves, size_t count)
{
    Game game;
    game_reset(&game, WHITE);
    for (size_t i = 0; i < count; ++i) {
        assert(game_play(&game, moves[i]));
    }
    return game;
}

static void assert_move(Move actual, Move expected)
{
    assert(actual.row == expected.row && actual.col == expected.col);
}

static void test_tactics(void)
{
    Game game;
    game_reset(&game, WHITE);
    assert_move(ai_search(&game, EASY, NULL, NULL).move, (Move){7, 7});

    /* Both players can win immediately and the AI has the next move. */
    const Move both_threaten[] = {
        {0, 0}, {2, 0}, {0, 1}, {2, 1}, {0, 2}, {2, 2}, {0, 3}, {2, 3}
    };
    game = replay(both_threaten, sizeof(both_threaten) / sizeof(both_threaten[0]));
    assert_move(ai_search(&game, EASY, NULL, NULL).move, (Move){0, 4});

    /* An internal gap is the opponent's only winning intersection. */
    const Move broken_four[] = {
        {0, 0}, {14, 14}, {0, 1}, {11, 9}, {0, 3}, {8, 6}, {0, 4}
    };
    game = replay(broken_four, sizeof(broken_four) / sizeof(broken_four[0]));
    assert_move(ai_search(&game, EASY, NULL, NULL).move, (Move){0, 2});

    const Move open_four[] = {
        {2, 3}, {14, 14}, {3, 4}, {11, 9}, {4, 5}, {8, 6}, {5, 6}
    };
    game = replay(open_four, sizeof(open_four) / sizeof(open_four[0]));
    const AiResult desperate = ai_search(&game, HARD, NULL, NULL);
    assert(game_can_play(&game, desperate.move));
    assert(game_winning_line(&game, desperate.move, BLACK, NULL, NULL));
    assert(game_play(&game, desperate.move));
    const Move remaining_win = desperate.move.row == 1 ? (Move){6, 7} : (Move){1, 2};
    assert(game_play(&game, remaining_win));
    assert(game.winner == BLACK);
    assert(!game_in_bounds(ai_search(&game, HARD, NULL, NULL).move));
}

static Game middle_game(void)
{
    const Move moves[] = {
        {7, 7}, {8, 8}, {6, 8}, {8, 6}, {8, 7}, {6, 7}, {7, 8}, {7, 6},
        {6, 5}, {8, 9}, {8, 5}, {7, 5}, {7, 4}, {9, 6}, {10, 6}, {6, 9}
    };
    return replay(moves, sizeof(moves) / sizeof(moves[0]));
}

static bool count_cancellation(void *context)
{
    int *polls_left = context;
    return --*polls_left <= 0;
}

static void test_search_and_cancellation(void)
{
    const Game game = middle_game();
    unsigned char snapshot[sizeof(game)];
    memcpy(snapshot, &game, sizeof(game));
    for (Difficulty level = EASY; level <= HARD; ++level) {
        const AiResult result = ai_search(&game, level, NULL, NULL);
        assert(!result.cancelled && game_can_play(&game, result.move));
        assert(result.depth > 0 && result.nodes > 0 && result.elapsed_ms >= 0.0);
        assert(memcmp(snapshot, &game, sizeof(game)) == 0);
        printf("level %d: depth %d, %llu nodes, %.1f ms\n",
               level, result.depth, result.nodes, result.elapsed_ms);
    }

    int polls_left = 40;
    const AiResult cancelled = ai_search(&game, HARD, count_cancellation, &polls_left);
    assert(cancelled.cancelled && !game_in_bounds(cancelled.move));
    assert(cancelled.nodes > 0);
    assert(memcmp(snapshot, &game, sizeof(game)) == 0);

    const AiResult timed_out = search_with_limits(&game, (Limits){8, 16, 0.0}, NULL, NULL);
    assert(!timed_out.cancelled && game_can_play(&game, timed_out.move));
    assert(timed_out.depth == 0 && timed_out.nodes == 0);
    assert(memcmp(snapshot, &game, sizeof(game)) == 0);
}

static int unpruned(Search *search, Game *board, int occupied, Stone side, Move previous,
                    int depth, int ply)
{
    ++search->nodes;
    const Stone opponent = game_other(side);
    if (game_winning_line(board, previous, opponent, NULL, NULL)) {
        return terminal_score(search, opponent, ply);
    }
    if (occupied == BOARD_CELLS) return 0;
    if (depth == 0) return evaluate(search, board, side, ply);

    Candidates list;
    generate_candidates(search, board, side, &list);
    if (list.wins > 0) return terminal_score(search, side, ply + 1);
    if (list.blocks > 1) return terminal_score(search, opponent, ply + 2);
    const int count = list.blocks == 1 ? 1 :
        list.count < search->width ? list.count : search->width;
    const bool maximizing = side == search->ai;
    int best = maximizing ? -SCORE_BOUND : SCORE_BOUND;
    for (int i = 0; i < count; ++i) {
        const Move move = list.items[i].move;
        board->cells[move.row][move.col] = side;
        const int score = unpruned(search, board, occupied + 1, opponent, move, depth - 1, ply + 1);
        board->cells[move.row][move.col] = EMPTY;
        if (maximizing ? score > best : score < best) best = score;
    }
    return best;
}

static void test_pruning(void)
{
    const Move moves[] = {{7, 7}, {8, 8}, {7, 6}, {6, 7}};
    Game game = replay(moves, sizeof(moves) / sizeof(moves[0]));
    Search pruned = {.ai = game.turn, .width = 5};
    init_clock(&pruned);
    pruned.deadline = now_ms(&pruned) + 10000.0;
    Search reference = pruned;
    const Move previous = game.history[game.count - 1];
    const int actual = minimax(&pruned, &game, game.count, game.turn, previous, 3, 0,
                               -SCORE_BOUND, SCORE_BOUND);
    const int expected = unpruned(&reference, &game, game.count, game.turn, previous, 3, 0);
    assert(!pruned.stopped && !reference.stopped);
    assert(actual == expected);
    assert(pruned.nodes < reference.nodes);
    printf("same-tree score %d, alpha-beta %llu nodes, reference %llu nodes\n",
           actual, pruned.nodes, reference.nodes);
}

int main(void)
{
    test_tactics();
    test_search_and_cancellation();
    test_pruning();
    puts("AI tests passed");
    return 0;
}

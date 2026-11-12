#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include "ai.h"

#include <stdlib.h>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <time.h>
#endif

enum {
    DIRECTION_COUNT = 4,
    WIN_SCORE = 100000000,
    SCORE_BOUND = 2 * WIN_SCORE
};

static const Move directions[DIRECTION_COUNT] = {
    {0, 1}, {1, 0}, {1, 1}, {1, -1}
};

typedef struct {
    int depth;
    int width;
    double budget_ms;
} Limits;

static const Limits limits_by_difficulty[] = {
    [EASY] = {2, 8, 200.0},
    [NORMAL] = {4, 12, 800.0},
    [HARD] = {8, 16, 2000.0}
};

typedef struct {
    Move move;
    int priority;
    int score;
} Candidate;

enum { QUIET, BLOCK, WIN };

typedef struct {
    Candidate items[BOARD_CELLS];
    int count;
    int wins;
    int blocks;
} Candidates;

typedef struct {
    Stone ai;
    int width;
    double deadline;
    unsigned long long nodes;
    AiCancelled check_cancelled;
    void *context;
    bool stopped;
    bool cancelled;
#ifdef _WIN32
    double milliseconds_per_tick;
#endif
} Search;

static void init_clock(Search *search)
{
#ifdef _WIN32
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    search->milliseconds_per_tick = 1000.0 / (double)frequency.QuadPart;
#else
    (void)search;
#endif
}

static double now_ms(const Search *search)
{
#ifdef _WIN32
    LARGE_INTEGER ticks;
    QueryPerformanceCounter(&ticks);
    return (double)ticks.QuadPart * search->milliseconds_per_tick;
#else
    struct timespec now;
    (void)search;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (double)now.tv_sec * 1000.0 + (double)now.tv_nsec / 1000000.0;
#endif
}

static bool should_stop(Search *search)
{
    if (search->check_cancelled && search->check_cancelled(search->context)) {
        search->cancelled = true;
    }
    search->stopped = search->stopped || search->cancelled ||
                      now_ms(search) >= search->deadline;
    return search->stopped;
}

static Move offset(Move move, Move direction, int distance)
{
    return (Move){move.row + direction.row * distance,
                  move.col + direction.col * distance};
}

static bool has_stone(const Game *board, Move move, Stone stone)
{
    return game_in_bounds(move) && board->cells[move.row][move.col] == stone;
}

static int run_length(const Game *board, Move start, Move direction, Stone stone)
{
    int count = 0;
    while (has_stone(board, offset(start, direction, count), stone)) {
        ++count;
    }
    return count;
}

static int run_score(int length, int open_ends)
{
    static const int scores[5][3] = {
        {0, 0, 0}, {0, 1, 6}, {0, 20, 160}, {0, 310, 3600}, {0, 12000, 60000}
    };
    return length >= 5 ? WIN_SCORE : scores[length][open_ends];
}

static int gap_score(int length, bool outer_end_open)
{
    if (length >= 4) return 12000;
    if (length == 3 && outer_end_open) return 650;
    return 0;
}

static int placement_score(const Game *board, Move move, Stone stone)
{
    int total = 0;
    for (int i = 0; i < DIRECTION_COUNT; ++i) {
        const Move direction = directions[i];
        const Move back = {-direction.row, -direction.col};
        const int left = run_length(board, offset(move, back, 1), back, stone);
        const int right = run_length(board, offset(move, direction, 1), direction, stone);
        const int length = 1 + left + right;
        if (length >= 5) return WIN_SCORE;

        const Move left_end = offset(move, back, left + 1);
        const Move right_end = offset(move, direction, right + 1);
        const bool left_open = has_stone(board, left_end, EMPTY);
        const bool right_open = has_stone(board, right_end, EMPTY);
        int score = run_score(length, left_open + right_open);

        if (left_open) {
            const int extension = run_length(board, offset(left_end, back, 1), back, stone);
            if (extension > 0) {
                const int broken = gap_score(length + extension, right_open ||
                    has_stone(board, offset(left_end, back, extension + 1), EMPTY));
                if (broken > score) score = broken;
            }
        }
        if (right_open) {
            const int extension = run_length(board, offset(right_end, direction, 1), direction, stone);
            if (extension > 0) {
                const int broken = gap_score(length + extension, left_open ||
                    has_stone(board, offset(right_end, direction, extension + 1), EMPTY));
                if (broken > score) score = broken;
            }
        }
        total += score;
    }
    return total;
}

static int compare_candidates(const void *first, const void *second)
{
    const Candidate *a = first;
    const Candidate *b = second;
    if (a->priority != b->priority) return b->priority - a->priority;
    if (a->score != b->score) return b->score - a->score;
    if (a->move.row != b->move.row) return a->move.row - b->move.row;
    return a->move.col - b->move.col;
}

static void generate_candidates(Search *search, const Game *board, Stone side, Candidates *list)
{
    bool nearby[BOARD_SIZE][BOARD_SIZE] = {{false}};
    list->count = 0;
    list->wins = 0;
    list->blocks = 0;

    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            if (board->cells[row][col] == EMPTY) continue;
            /* Every one-move win is adjacent to a stone in its winning line. */
            for (int dr = -2; dr <= 2; ++dr) {
                for (int dc = -2; dc <= 2; ++dc) {
                    const Move move = {row + dr, col + dc};
                    if (game_in_bounds(move)) nearby[move.row][move.col] = true;
                }
            }
        }
    }

    const Stone opponent = game_other(side);
    for (int row = 0; row < BOARD_SIZE; ++row) {
        if (should_stop(search)) return;
        for (int col = 0; col < BOARD_SIZE; ++col) {
            if (!nearby[row][col] || board->cells[row][col] != EMPTY) continue;

            const Move move = {row, col};
            const int attack = placement_score(board, move, side);
            const int defense = placement_score(board, move, opponent);
            const int priority = attack == WIN_SCORE ? WIN : defense == WIN_SCORE ? BLOCK : QUIET;
            const int center_distance = abs(row - BOARD_SIZE / 2) + abs(col - BOARD_SIZE / 2);
            const int score = 3 * attack + 2 * defense - center_distance;
            list->items[list->count++] = (Candidate){move, priority, score};
            list->wins += priority == WIN;
            list->blocks += defense == WIN_SCORE;
        }
    }
    qsort(list->items, (size_t)list->count, sizeof(list->items[0]), compare_candidates);
}

static int terminal_score(const Search *search, Stone winner, int ply)
{
    return winner == search->ai ? WIN_SCORE - ply : ply - WIN_SCORE;
}

static int evaluate(Search *search, const Game *board, Stone side, int ply)
{
    int scores[3] = {0};
    bool immediate[3] = {false};
    for (int row = 0; row < BOARD_SIZE; ++row) {
        if (should_stop(search)) return 0;
        for (int col = 0; col < BOARD_SIZE; ++col) {
            const Stone stone = board->cells[row][col];
            if (stone == EMPTY) continue;

            const Move start = {row, col};
            for (int i = 0; i < DIRECTION_COUNT; ++i) {
                const Move direction = directions[i];
                const Move before = offset(start, direction, -1);
                if (has_stone(board, before, stone)) continue;

                const int length = run_length(board, start, direction, stone);
                if (length >= 5) return terminal_score(search, stone, ply);
                const Move after = offset(start, direction, length);
                const bool before_open = has_stone(board, before, EMPTY);
                const bool after_open = has_stone(board, after, EMPTY);
                scores[stone] += run_score(length, before_open + after_open);
                immediate[stone] |= length == 4 && (before_open || after_open);

                if (after_open) {
                    const Move next = offset(after, direction, 1);
                    const int extension = run_length(board, next, direction, stone);
                    if (extension == 0) continue;
                    const int total = length + extension;
                    const Move far_end = offset(next, direction, extension);
                    immediate[stone] |= total >= 4;
                    if (length < 4 && extension < 4) {
                        scores[stone] += gap_score(total, before_open ||
                                                   has_stone(board, far_end, EMPTY));
                    }
                }
            }
        }
    }
    if (immediate[side]) return terminal_score(search, side, ply + 1);
    return scores[search->ai] - scores[game_other(search->ai)];
}

static int minimax(Search *search, Game *board, int occupied, Stone side, Move previous,
                   int depth, int ply, int alpha, int beta);

static int search_moves(Search *search, Game *board, int occupied, Stone side,
                        const Candidates *list, int depth, int ply, int alpha, int beta,
                        Move *best_move)
{
    const bool maximizing = side == search->ai;
    int best = maximizing ? -SCORE_BOUND : SCORE_BOUND;
    int count = list->count < search->width ? list->count : search->width;
    if (list->wins > 0) count = list->wins;
    else if (list->blocks > 0) count = list->blocks;

    for (int i = 0; i < count; ++i) {
        const Move move = list->items[i].move;
        board->cells[move.row][move.col] = side;
        const int score = minimax(search, board, occupied + 1, game_other(side), move,
                                  depth - 1, ply + 1, alpha, beta);
        board->cells[move.row][move.col] = EMPTY;
        if (search->stopped) break;

        if (maximizing ? score > best : score < best) {
            best = score;
            if (best_move) *best_move = move;
        }
        if (maximizing && best > alpha) alpha = best;
        if (!maximizing && best < beta) beta = best;
        if (alpha >= beta) break;
    }
    return best;
}

static int minimax(Search *search, Game *board, int occupied, Stone side, Move previous,
                   int depth, int ply, int alpha, int beta)
{
    ++search->nodes;
    if (should_stop(search)) return 0;
    const Stone opponent = game_other(side);
    if (game_winning_line(board, previous, opponent, NULL, NULL)) {
        return terminal_score(search, opponent, ply);
    }
    if (occupied == BOARD_CELLS) return 0;
    if (depth == 0) return evaluate(search, board, side, ply);

    Candidates list;
    generate_candidates(search, board, side, &list);
    if (search->stopped) return 0;
    if (list.wins > 0) return terminal_score(search, side, ply + 1);
    if (list.blocks > 1) return terminal_score(search, opponent, ply + 2);
    return search_moves(search, board, occupied, side, &list, depth, ply, alpha, beta, NULL);
}

static Move fallback_move(const Game *position)
{
    Move best = {-1, -1};
    int best_distance = BOARD_SIZE * 2;
    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            const int distance = abs(row - BOARD_SIZE / 2) + abs(col - BOARD_SIZE / 2);
            if (distance < best_distance && position->cells[row][col] == EMPTY) {
                best = (Move){row, col};
                best_distance = distance;
            }
        }
    }
    return best;
}

static void promote_move(Candidates *list, Move move)
{
    for (int i = 1; i < list->count; ++i) {
        if (list->items[i].move.row == move.row && list->items[i].move.col == move.col) {
            const Candidate best = list->items[i];
            for (int j = i; j > 0; --j) list->items[j] = list->items[j - 1];
            list->items[0] = best;
            break;
        }
    }
}

static AiResult search_with_limits(const Game *position, Limits limits,
                                   AiCancelled cancelled, void *context)
{
    Search search = {.ai = position->turn, .width = limits.width,
                     .check_cancelled = cancelled, .context = context};
    init_clock(&search);
    const double start = now_ms(&search);
    search.deadline = start + limits.budget_ms;
    AiResult result = {.move = game_over(position) ? (Move){-1, -1} : fallback_move(position)};

    if (position->count > 0 && game_in_bounds(result.move) && !should_stop(&search)) {
        Game board = *position;
        Candidates list;
        generate_candidates(&search, &board, search.ai, &list);
        if (!search.stopped && list.count > 0) {
            result.move = list.items[0].move;
            if (list.wins > 0 || list.blocks > 1) {
                /* A direct win takes precedence over an unavoidable loss. */
                result.depth = 1;
                search.nodes = 1;
            } else {
                for (int depth = 1; depth <= limits.depth && !should_stop(&search); ++depth) {
                    Move best = result.move;
                    ++search.nodes;
                    const int score = search_moves(&search, &board, position->count, search.ai,
                        &list, depth, 0, -SCORE_BOUND, SCORE_BOUND, &best);
                    if (search.stopped) break;
                    result.move = best;
                    result.depth = depth;
                    if (score >= WIN_SCORE - depth) break;
                    promote_move(&list, best);
                }
            }
        }
    }

    should_stop(&search);
    result.cancelled = search.cancelled;
    if (result.cancelled) result.move = (Move){-1, -1};
    result.nodes = search.nodes;
    result.elapsed_ms = now_ms(&search) - start;
    return result;
}

AiResult ai_search(const Game *position, Difficulty difficulty,
                   AiCancelled cancelled, void *context)
{
    const Limits limits = limits_by_difficulty[difficulty >= EASY && difficulty <= HARD ?
                                              difficulty : NORMAL];
    return search_with_limits(position, limits, cancelled, context);
}

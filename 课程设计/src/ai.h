#ifndef GOBANG_AI_H
#define GOBANG_AI_H

#include "game.h"

typedef enum { EASY, NORMAL, HARD } Difficulty;

typedef struct {
    Move move;
    int depth;
    unsigned long long nodes;
    double elapsed_ms;
    bool cancelled;
} AiResult;

typedef bool (*AiCancelled)(void *context);

/* Search for position->turn. A cancelled search has no usable result. */
AiResult ai_search(const Game *position, Difficulty difficulty,
                   AiCancelled cancelled, void *context);

#endif

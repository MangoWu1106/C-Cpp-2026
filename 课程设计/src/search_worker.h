#ifndef GOBANG_SEARCH_WORKER_H
#define GOBANG_SEARCH_WORKER_H

#include <stdint.h>

#include "ai.h"

typedef struct SearchWorker SearchWorker;

SearchWorker *search_worker_create(void);
void search_worker_destroy(SearchWorker *worker);
uint64_t search_worker_submit(SearchWorker *worker, const Game *game, Difficulty difficulty);
void search_worker_cancel(SearchWorker *worker);
bool search_worker_poll(SearchWorker *worker, uint64_t *request, AiResult *result);

#endif

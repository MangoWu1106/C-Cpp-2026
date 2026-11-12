#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0600
#include <windows.h>

#include <process.h>
#include <stdlib.h>

#include "search_worker.h"

struct SearchWorker {
    HANDLE thread;
    CRITICAL_SECTION lock;
    CONDITION_VARIABLE wake;
    bool stopping;
    bool queued;
    bool ready;
    uint64_t request;
    Game snapshot;
    Difficulty difficulty;
    AiResult result;
};

typedef struct {
    SearchWorker *worker;
    uint64_t request;
} Cancellation;

static bool is_cancelled(void *context)
{
    Cancellation *cancellation = context;
    SearchWorker *worker = cancellation->worker;

    EnterCriticalSection(&worker->lock);
    bool cancelled = worker->stopping || cancellation->request != worker->request;
    LeaveCriticalSection(&worker->lock);
    return cancelled;
}

static unsigned int __stdcall run_searches(void *context)
{
    SearchWorker *worker = context;
    EnterCriticalSection(&worker->lock);
    while (!worker->stopping) {
        if (!worker->queued) {
            SleepConditionVariableCS(&worker->wake, &worker->lock, INFINITE);
            continue;
        }

        Game snapshot = worker->snapshot;
        Difficulty difficulty = worker->difficulty;
        Cancellation cancellation = { worker, worker->request };
        worker->queued = false;
        LeaveCriticalSection(&worker->lock);

        AiResult result = ai_search(&snapshot, difficulty, is_cancelled, &cancellation);

        EnterCriticalSection(&worker->lock);
        if (!worker->stopping && !result.cancelled && cancellation.request == worker->request) {
            worker->result = result;
            worker->ready = true;
        }
    }
    LeaveCriticalSection(&worker->lock);
    return 0;
}

SearchWorker *search_worker_create(void)
{
    SearchWorker *worker = calloc(1, sizeof *worker);
    if (!worker) {
        return NULL;
    }
    InitializeCriticalSection(&worker->lock);
    InitializeConditionVariable(&worker->wake);
    worker->thread = (HANDLE)_beginthreadex(NULL, 0, run_searches, worker, 0, NULL);
    if (!worker->thread) {
        DeleteCriticalSection(&worker->lock);
        free(worker);
        return NULL;
    }
    return worker;
}

void search_worker_destroy(SearchWorker *worker)
{
    EnterCriticalSection(&worker->lock);
    worker->stopping = true;
    WakeConditionVariable(&worker->wake);
    LeaveCriticalSection(&worker->lock);

    WaitForSingleObject(worker->thread, INFINITE);
    CloseHandle(worker->thread);
    DeleteCriticalSection(&worker->lock);
    free(worker);
}

uint64_t search_worker_submit(SearchWorker *worker, const Game *game, Difficulty difficulty)
{
    EnterCriticalSection(&worker->lock);
    uint64_t request = ++worker->request;
    worker->snapshot = *game;
    worker->difficulty = difficulty;
    worker->queued = true;
    worker->ready = false;
    WakeConditionVariable(&worker->wake);
    LeaveCriticalSection(&worker->lock);
    return request;
}

void search_worker_cancel(SearchWorker *worker)
{
    EnterCriticalSection(&worker->lock);
    ++worker->request;
    worker->queued = false;
    worker->ready = false;
    LeaveCriticalSection(&worker->lock);
}

bool search_worker_poll(SearchWorker *worker, uint64_t *request, AiResult *result)
{
    EnterCriticalSection(&worker->lock);
    bool ready = worker->ready;
    if (ready) {
        *request = worker->request;
        *result = worker->result;
        worker->ready = false;
    }
    LeaveCriticalSection(&worker->lock);
    return ready;
}

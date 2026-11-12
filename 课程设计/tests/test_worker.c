#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>

#include "search_worker.h"

static HANDLE entered_search;
static HANDLE release_search;

/* Hold the old search until a replacement is queued, then return a stale move. */
AiResult ai_search(const Game *position, Difficulty difficulty, AiCancelled cancelled, void *context)
{
    if (difficulty == HARD) {
        assert(SetEvent(entered_search));
        assert(WaitForSingleObject(release_search, 5000) == WAIT_OBJECT_0);
        assert(cancelled(context));
        return (AiResult){ .move = { 0, 0 } };
    }
    assert(!cancelled(context));
    assert(position->count == 1);
    assert(position->cells[7][7] == BLACK);
    return (AiResult){ .move = { 6, 7 } };
}

int main(void)
{
    entered_search = CreateEventW(NULL, FALSE, FALSE, NULL);
    release_search = CreateEventW(NULL, FALSE, FALSE, NULL);
    assert(entered_search && release_search);

    SearchWorker *worker = search_worker_create();
    assert(worker);
    Game game;
    game_reset(&game, BLACK);
    uint64_t old_request = search_worker_submit(worker, &game, HARD);
    assert(WaitForSingleObject(entered_search, 5000) == WAIT_OBJECT_0);
    search_worker_cancel(worker);

    assert(game_play(&game, (Move){ 7, 7 }));
    uint64_t new_request = search_worker_submit(worker, &game, EASY);
    assert(old_request != new_request);
    game_reset(&game, BLACK);
    assert(SetEvent(release_search));

    uint64_t request = 0;
    AiResult result;
    ULONGLONG end = GetTickCount64() + 5000;
    while (!search_worker_poll(worker, &request, &result) && GetTickCount64() < end) {
        Sleep(1);
    }
    assert(request == new_request);
    assert(result.move.row == 6 && result.move.col == 7);
    assert(!search_worker_poll(worker, &request, &result));

    search_worker_destroy(worker);
    CloseHandle(release_search);
    CloseHandle(entered_search);
    puts("Worker cancellation and snapshot tests passed");
    return 0;
}

#include <stdlib.h>

#include "ui.h"
#include "assets.h"
#include "search_worker.h"

typedef struct {
    Game game;
    Ui ui;
    UiScreen screen;
    UiConfirmation confirmation;
    Stone player;
    Difficulty difficulty;
    SearchWorker *worker;
    uint64_t ai_request;
    bool quit;
} App;

static Font load_font(void)
{
    int data_size = 0;
    unsigned char *data = assets_read_font(&data_size);
    if (!data) {
        return (Font){ 0 };
    }

    int count = 0;
    int *codepoints = LoadCodepoints(ui_font_text(), &count);
    int unique = 0;
    for (int i = 0; i < count; ++i) {
        int codepoint = codepoints[i];
        int j = 0;
        while (j < unique && codepoints[j] != codepoint) {
            ++j;
        }
        if (j == unique && codepoint >= ' ' && codepoint != 0x7f) {
            codepoints[unique++] = codepoint;
        }
    }

    Font font = { 0 };
    if (unique > 0) {
        font = LoadFontFromMemory(".otf", data, data_size, 56, codepoints, unique);
        if (font.texture.id == GetFontDefault().texture.id) {
            font = (Font){ 0 };
        } else if (font.texture.id) {
            SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
        }
    }
    UnloadCodepoints(codepoints);
    free(data);
    return font;
}

static UiView app_view(const App *app)
{
    return (UiView){ app->screen, app->confirmation, &app->game, app->player, app->difficulty };
}

static void show_screen(App *app, UiScreen screen)
{
    app->screen = screen;
    app->confirmation = UI_CONFIRM_NONE;
    app->ui = (Ui){ 0 };
}

static void cancel_search(App *app)
{
    search_worker_cancel(app->worker);
    app->ai_request = 0;
}

static void start_match(App *app)
{
    cancel_search(app);
    game_reset(&app->game, app->player);
    show_screen(app, UI_MATCH);
}

static void return_to_menu(App *app)
{
    cancel_search(app);
    show_screen(app, UI_MENU);
}

static bool match_in_progress(const App *app)
{
    return app->screen == UI_MATCH && app->game.count > 0 && !game_over(&app->game);
}

static void request_exit(App *app)
{
    if (match_in_progress(app)) {
        app->confirmation = UI_CONFIRM_EXIT;
    } else {
        app->quit = true;
    }
}

static void confirm_action(App *app)
{
    UiConfirmation action = app->confirmation;
    app->confirmation = UI_CONFIRM_NONE;
    switch (action) {
        case UI_CONFIRM_RESTART: start_match(app); break;
        case UI_CONFIRM_MENU: return_to_menu(app); break;
        case UI_CONFIRM_EXIT: app->quit = true; break;
        case UI_CONFIRM_NONE: break;
    }
}

static void handle_action(App *app, UiAction action)
{
    switch (action.command) {
        case UI_START:
            start_match(app);
            break;
        case UI_OPEN_SETTINGS:
            show_screen(app, UI_SETTINGS);
            break;
        case UI_OPEN_RULES:
            show_screen(app, UI_RULES);
            break;
        case UI_BACK:
            return_to_menu(app);
            break;
        case UI_EXIT:
            request_exit(app);
            break;
        case UI_SET_STONE:
            app->player = action.player;
            break;
        case UI_SET_DIFFICULTY:
            app->difficulty = action.difficulty;
            break;
        case UI_PLACE:
            if (app->screen == UI_MATCH && app->game.turn == app->game.player) {
                game_play(&app->game, action.move);
            }
            break;
        case UI_UNDO:
            if (game_can_undo(&app->game)) {
                cancel_search(app);
                game_undo(&app->game);
            }
            break;
        case UI_RESTART:
            if (match_in_progress(app)) {
                app->confirmation = UI_CONFIRM_RESTART;
            } else {
                start_match(app);
            }
            break;
        case UI_RETURN_MENU:
            if (match_in_progress(app)) {
                app->confirmation = UI_CONFIRM_MENU;
            } else {
                return_to_menu(app);
            }
            break;
        case UI_CONFIRM:
            confirm_action(app);
            break;
        case UI_CANCEL:
            app->confirmation = UI_CONFIRM_NONE;
            break;
        case UI_NONE:
            break;
    }
}

static void advance_ai(App *app)
{
    if (app->screen != UI_MATCH || app->confirmation != UI_CONFIRM_NONE ||
        game_over(&app->game) || app->game.turn == app->game.player) {
        return;
    }

    if (!app->ai_request) {
        app->ai_request = search_worker_submit(app->worker, &app->game, app->difficulty);
        return;
    }

    uint64_t request;
    AiResult result;
    if (search_worker_poll(app->worker, &request, &result) && request == app->ai_request) {
        app->ai_request = 0;
        if (game_play(&app->game, result.move)) {
            TraceLog(LOG_DEBUG, "AI: depth %d, nodes %llu, %.2f ms",
                     result.depth, result.nodes, result.elapsed_ms);
        }
    }
}

int main(void)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(1100, 800, "五子棋 · 人机对战");
    if (!IsWindowReady()) {
        assets_show_error(L"无法打开游戏窗口。");
        return EXIT_FAILURE;
    }
    SetWindowMinSize(780, 590);
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);

    Font font = load_font();
    if (!IsFontValid(font) || !font.texture.id) {
        if (IsFontValid(font)) {
            UnloadFont(font);
        }
        CloseWindow();
        assets_show_error(L"无法加载中文字体。请确认 assets 文件夹与 Gobang.exe 在同一目录。");
        return EXIT_FAILURE;
    }

    App app = { .screen = UI_MENU, .player = BLACK, .difficulty = NORMAL };
    app.worker = search_worker_create();
    if (!app.worker) {
        UnloadFont(font);
        CloseWindow();
        assets_show_error(L"无法启动电脑对手。");
        return EXIT_FAILURE;
    }

    while (!app.quit) {
        if (WindowShouldClose()) {
            request_exit(&app);
        } else {
            UiView view = app_view(&app);
            handle_action(&app, ui_update(&app.ui, &view));
        }
        if (app.quit) {
            break;
        }
        advance_ai(&app);

        UiView view = app_view(&app);
        BeginDrawing();
        ui_draw(&app.ui, &view, font);
        EndDrawing();
    }

    search_worker_destroy(app.worker);
    UnloadFont(font);
    CloseWindow();
    return EXIT_SUCCESS;
}

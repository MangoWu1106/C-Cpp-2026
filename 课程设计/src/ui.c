#include "ui.h"

#include <math.h>
#include <stddef.h>

#define UI_TEXT(X) \
    X(BRAND, "五子棋") \
    X(BRAND_DETAIL, "本地人机对战") \
    X(MENU_TITLE, "黑白之间") \
    X(MENU_SUBTITLE, "一盘五子棋，和电脑比一比。") \
    X(MENU_BOARD, "15 × 15") \
    X(MENU_RULE, "自由规则 · 连五获胜") \
    X(MENU_PICK, "开始对局") \
    X(MENU_PICK_NOTE, "按当前设置开始一局。") \
    X(START, "开始游戏") \
    X(SETTINGS, "游戏设置") \
    X(RULES, "规则说明") \
    X(EXIT, "退出游戏") \
    X(SETTINGS_NOTE, "这些设置将在下一局生效。") \
    X(SELECT_STONE, "选择执子") \
    X(SELECT_DIFFICULTY, "选择难度") \
    X(BLACK_NAME, "黑棋") \
    X(WHITE_NAME, "白棋") \
    X(BLACK_FIRST, "黑棋先手") \
    X(WHITE_SECOND, "白棋后手") \
    X(EASY_NAME, "简单") \
    X(NORMAL_NAME, "普通") \
    X(HARD_NAME, "困难") \
    X(BACK, "返回主菜单") \
    X(RULE_TURN, "轮流在空交叉点落一枚棋子，黑棋先手。") \
    X(RULE_WIN, "横、竖或斜向连续五子或以上即获胜。") \
    X(RULE_FREE, "双方均无禁手，六连或更长也算获胜。") \
    X(RULE_DRAW, "棋盘下满且双方均未获胜时判为平局。") \
    X(STATUS_LABEL, "对局状态") \
    X(YOUR_TURN, "轮到你落子") \
    X(THINKING, "电脑思考中……") \
    X(YOU_WIN, "你赢了") \
    X(COMPUTER_WINS, "电脑获胜") \
    X(DRAW_GAME, "平局") \
    X(YOUR_TURN_NOTE, "选择一个空交叉点。") \
    X(THINKING_NOTE, "电脑正在寻找下一步。") \
    X(FINISHED_NOTE, "本局已结束。") \
    X(YOU, "你") \
    X(COMPUTER, "电脑") \
    X(DIFFICULTY_LABEL, "难度") \
    X(UNDO, "悔棋") \
    X(RESTART, "重新开始") \
    X(REPLAY, "再来一局") \
    X(RETURN_MENU, "返回菜单") \
    X(CONFIRM_RESTART, "重新开始这局？") \
    X(CONFIRM_MENU, "返回主菜单？") \
    X(CONFIRM_EXIT, "退出游戏？") \
    X(CONFIRM_LOSS, "当前棋局将丢失。") \
    X(CANCEL, "取消") \
    X(CONFIRM, "确认") \
    X(MENU_KEYS, "方向键或 Tab 切换 · Enter 确认 · Esc 返回") \
    X(MATCH_KEYS, "方向键移动棋盘焦点 · Enter 落子 · Tab 切换操作") \
    X(LAST_MOVE, "最近落子")

#define TEXT_CONSTANT(name, value) static const char T_##name[] = value;
UI_TEXT(TEXT_CONSTANT)
#undef TEXT_CONSTANT

const char *ui_font_text(void)
{
#define TEXT_VALUE(name, value) value
    return UI_TEXT(TEXT_VALUE) " 0123456789ABCDEFGHIJKLMNO";
#undef TEXT_VALUE
}

static const Color background = { 240, 238, 228, 255 };
static const Color paper = { 252, 250, 241, 255 };
static const Color ink = { 29, 43, 37, 255 };
static const Color muted = { 84, 98, 89, 255 };
static const Color forest = { 44, 76, 64, 255 };
static const Color dark_forest = { 31, 63, 52, 255 };
static const Color sage = { 228, 236, 224, 255 };
static const Color outline = { 208, 214, 201, 255 };
static const Color ivory = { 250, 245, 226, 255 };
static const Color red = { 167, 66, 47, 255 };
static const Color wood = { 216, 176, 122, 255 };
static const Color wood_dark = { 116, 78, 48, 255 };

static const Rectangle board_area = { 48, 136, 584, 584 };
static const Rectangle match_panel = { 656, 136, 396, 584 };
static const Rectangle page_panel = { 188, 148, 724, 552 };
static const Rectangle dialog = { 308, 276, 484, 248 };

typedef enum { BUTTON_PLAIN, BUTTON_PRIMARY, BUTTON_CHOICE } ButtonStyle;

typedef struct {
    Rectangle rect;
    const char *label;
    UiAction action;
    ButtonStyle style;
    bool enabled;
    bool selected;
} Button;

static Camera2D viewport(void)
{
    const float width = (float)GetScreenWidth();
    const float height = (float)GetScreenHeight();
    const float scale = fmaxf(0.01f, fminf(width / 1100.0f, height / 800.0f));
    return (Camera2D){ .offset = { (width - 1100.0f * scale) / 2.0f,
                                  (height - 800.0f * scale) / 2.0f },
                       .zoom = scale };
}

static Rectangle inset(Rectangle rect, float amount)
{
    return (Rectangle){ rect.x + amount, rect.y + amount,
                        rect.width - 2 * amount, rect.height - 2 * amount };
}

static const char *stone_name(Stone stone)
{
    return stone == BLACK ? T_BLACK_NAME : T_WHITE_NAME;
}

static const char *difficulty_name(Difficulty difficulty)
{
    if (difficulty == EASY) return T_EASY_NAME;
    if (difficulty == HARD) return T_HARD_NAME;
    return T_NORMAL_NAME;
}

static Vector2 intersection(Rectangle area, int size, float padding, Move move)
{
    const float step = (area.width - 2 * padding) / (float)(size - 1);
    return (Vector2){ area.x + padding + (float)move.col * step,
                      area.y + padding + (float)move.row * step };
}

static Move board_move_at(Vector2 mouse)
{
    const float padding = 38;
    const float step = (board_area.width - 2 * padding) / (BOARD_SIZE - 1);
    const float x = (mouse.x - board_area.x - padding) / step;
    const float y = (mouse.y - board_area.y - padding) / step;
    const Move move = { (int)roundf(y), (int)roundf(x) };
    const float dx = x - (float)move.col;
    const float dy = y - (float)move.row;
    if (game_in_bounds(move) && dx * dx + dy * dy <= 0.43f * 0.43f) return move;
    return (Move){ -1, -1 };
}

static bool player_can_place(const Game *game, Move move)
{
    return game->turn == game->player && game_can_play(game, move);
}

static Button button(Rectangle rect, const char *label, UiCommand command,
                     ButtonStyle style)
{
    return (Button){ .rect = rect, .label = label, .action.command = command,
                     .style = style, .enabled = true };
}

static int build_buttons(const UiView *view, Button buttons[6])
{
    if (view->confirmation != UI_CONFIRM_NONE) {
        buttons[0] = button((Rectangle){ 340, 438, 202, 54 }, T_CANCEL, UI_CANCEL, BUTTON_PLAIN);
        buttons[1] = button((Rectangle){ 558, 438, 202, 54 }, T_CONFIRM, UI_CONFIRM, BUTTON_PRIMARY);
        return 2;
    }

    switch (view->screen) {
        case UI_MENU:
            buttons[0] = button((Rectangle){ 708, 310, 316, 65 }, T_START, UI_START, BUTTON_PRIMARY);
            buttons[1] = button((Rectangle){ 708, 389, 316, 58 }, T_SETTINGS, UI_OPEN_SETTINGS, BUTTON_PLAIN);
            buttons[2] = button((Rectangle){ 708, 459, 316, 58 }, T_RULES, UI_OPEN_RULES, BUTTON_PLAIN);
            buttons[3] = button((Rectangle){ 708, 529, 316, 58 }, T_EXIT, UI_EXIT, BUTTON_PLAIN);
            return 4;
        case UI_SETTINGS:
            buttons[0] = button((Rectangle){ 224, 342, 316, 60 }, T_BLACK_FIRST, UI_SET_STONE, BUTTON_CHOICE);
            buttons[1] = button((Rectangle){ 554, 342, 322, 60 }, T_WHITE_SECOND, UI_SET_STONE, BUTTON_CHOICE);
            buttons[0].action.player = BLACK;
            buttons[1].action.player = WHITE;
            buttons[0].selected = view->player == BLACK;
            buttons[1].selected = view->player == WHITE;
            for (int i = 0; i < 3; ++i) {
                const Difficulty difficulty = (Difficulty)i;
                const Rectangle rect = { 224 + (float)i * 220, 466, 212, 60 };
                buttons[i + 2] = button(rect, difficulty_name(difficulty), UI_SET_DIFFICULTY, BUTTON_CHOICE);
                buttons[i + 2].action.difficulty = difficulty;
                buttons[i + 2].selected = view->difficulty == difficulty;
            }
            buttons[5] = button((Rectangle){ 224, 600, 652, 56 }, T_BACK, UI_BACK, BUTTON_PRIMARY);
            return 6;
        case UI_RULES:
            buttons[0] = button((Rectangle){ 224, 600, 652, 56 }, T_BACK, UI_BACK, BUTTON_PRIMARY);
            return 1;
        case UI_MATCH:
            buttons[0] = button(board_area, NULL, UI_PLACE, BUTTON_PLAIN);
            buttons[0].enabled = !game_over(view->game);
            buttons[1] = button((Rectangle){ 684, 482, 340, 57 }, T_UNDO, UI_UNDO, BUTTON_PLAIN);
            buttons[1].enabled = game_can_undo(view->game);
            if (game_over(view->game)) {
                buttons[2] = button((Rectangle){ 684, 551, 340, 57 }, T_REPLAY, UI_RESTART, BUTTON_PRIMARY);
            } else {
                buttons[2] = button((Rectangle){ 684, 551, 340, 57 }, T_RESTART, UI_RESTART, BUTTON_PLAIN);
            }
            buttons[3] = button((Rectangle){ 684, 620, 340, 57 }, T_RETURN_MENU, UI_RETURN_MENU, BUTTON_PLAIN);
            return 4;
    }
    return 0;
}

static int next_focus(const Button buttons[6], int count, int focus, int direction)
{
    do {
        focus = (focus + direction + count) % count;
    } while (!buttons[focus].enabled);
    return focus;
}

static UiCommand escape_command(const UiView *view)
{
    if (view->confirmation != UI_CONFIRM_NONE) return UI_CANCEL;
    if (view->screen == UI_MATCH) return UI_RETURN_MENU;
    if (view->screen == UI_MENU) return UI_EXIT;
    return UI_BACK;
}

UiAction ui_update(Ui *ui, const UiView *view)
{
    if (!ui->initialized) {
        ui->board_focus = (Move){ BOARD_SIZE / 2, BOARD_SIZE / 2 };
        ui->initialized = true;
    }

    const bool modal = view->confirmation != UI_CONFIRM_NONE;
    if (modal && !ui->modal_open) ui->modal_focus = 0;
    ui->modal_open = modal;
    if (IsKeyPressed(KEY_ESCAPE)) return (UiAction){ .command = escape_command(view) };

    Button buttons[6];
    const int count = build_buttons(view, buttons);
    int *focus = modal ? &ui->modal_focus : &ui->focus;
    if (!buttons[*focus].enabled) *focus = next_focus(buttons, count, *focus, 1);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        const Vector2 mouse = GetScreenToWorld2D(GetMousePosition(), viewport());
        ui->keyboard = false;
        for (int i = 0; i < count; ++i) {
            if (!buttons[i].enabled || !CheckCollisionPointRec(mouse, buttons[i].rect)) continue;
            if (buttons[i].action.command == UI_PLACE) {
                const Move move = board_move_at(mouse);
                if (!player_can_place(view->game, move)) break;
                ui->board_focus = move;
                ui->focus = 0;
                return (UiAction){ .command = UI_PLACE, .move = move };
            }
            *focus = i;
            return buttons[i].action;
        }
        return (UiAction){ 0 };
    }

    if (IsKeyPressed(KEY_TAB)) {
        const bool reverse = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        *focus = next_focus(buttons, count, *focus, reverse ? -1 : 1);
        ui->keyboard = true;
        return (UiAction){ 0 };
    }

    const int dx = (int)IsKeyPressed(KEY_RIGHT) - (int)IsKeyPressed(KEY_LEFT);
    const int dy = (int)IsKeyPressed(KEY_DOWN) - (int)IsKeyPressed(KEY_UP);
    if (dx != 0 || dy != 0) {
        ui->keyboard = true;
        if (buttons[*focus].action.command == UI_PLACE) {
            Move next = { ui->board_focus.row + dy, ui->board_focus.col + dx };
            if (next.row >= 0 && next.row < BOARD_SIZE) ui->board_focus.row = next.row;
            if (next.col >= 0 && next.col < BOARD_SIZE) ui->board_focus.col = next.col;
        } else {
            *focus = next_focus(buttons, count, *focus, dy != 0 ? dy : dx);
        }
        return (UiAction){ 0 };
    }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
        ui->keyboard = true;
        if (buttons[*focus].action.command != UI_PLACE) return buttons[*focus].action;
        if (player_can_place(view->game, ui->board_focus)) {
            return (UiAction){ .command = UI_PLACE, .move = ui->board_focus };
        }
    }
    return (UiAction){ 0 };
}

static void label(Font font, const char *value, float x, float y, float size, Color color)
{
    DrawTextEx(font, value, (Vector2){ x, y }, size, 1, color);
}

static void center_label(Font font, const char *value, Rectangle rect, float size, Color color)
{
    const Vector2 extent = MeasureTextEx(font, value, size, 1);
    label(font, value, rect.x + (rect.width - extent.x) / 2.0f,
          rect.y + (rect.height - size) / 2.0f, size, color);
}

static void horizontal_line(float x, float end, float y, Color color)
{
    DrawLineEx((Vector2){ x, y }, (Vector2){ end, y }, 1, color);
}

static void card(Rectangle rect, Color fill)
{
    const Rectangle shadow = { rect.x, rect.y + 4, rect.width, rect.height };
    DrawRectangleRounded(shadow, 0.05f, 8, Fade(ink, 0.07f));
    DrawRectangleRounded(rect, 0.05f, 8, outline);
    DrawRectangleRounded(inset(rect, 1), 0.05f, 8, fill);
}

static void draw_button(Font font, const Button *item, Vector2 mouse, bool focused)
{
    if (item->action.command == UI_PLACE) return;
    const bool hover = item->enabled && CheckCollisionPointRec(mouse, item->rect);
    Color fill = hover ? sage : paper;
    Color edge = outline;
    Color foreground = ink;

    if (item->style == BUTTON_PRIMARY) {
        fill = hover ? dark_forest : forest;
        edge = fill;
        foreground = ivory;
    } else if (item->selected) {
        fill = sage;
        edge = forest;
    }
    if (!item->enabled) {
        fill = background;
        foreground = muted;
    }

    if (focused && item->enabled) {
        DrawRectangleRounded(inset(item->rect, -4), 0.24f, 8, red);
        DrawRectangleRounded(inset(item->rect, -2), 0.24f, 8, paper);
    }
    DrawRectangleRounded(item->rect, 0.22f, 8, edge);
    DrawRectangleRounded(inset(item->rect, 1), 0.22f, 8, fill);
    if (item->selected) {
        DrawCircleV((Vector2){ item->rect.x + 24, item->rect.y + item->rect.height / 2 }, 4, forest);
    }
    center_label(font, item->label, item->rect, 24, foreground);
}

static void draw_frame(Font font, bool match)
{
    const Rectangle mark = { 48, 38, 54, 54 };
    DrawRectangleRounded(mark, 0.24f, 8, forest);
    DrawCircleV((Vector2){ 70, 60 }, 9, ivory);
    DrawCircleV((Vector2){ 80, 70 }, 9, dark_forest);
    DrawCircleLines(80, 70, 9, ivory);
    label(font, T_BRAND, 118, 36, 34, ink);
    label(font, T_BRAND_DETAIL, 119, 76, 17, muted);
    horizontal_line(48, 1052, 111, outline);
    label(font, match ? T_MATCH_KEYS : T_MENU_KEYS, 48, 751, 18, muted);
}

static void draw_wood(Rectangle area)
{
    const Rectangle shadow = { area.x, area.y + 4, area.width, area.height };
    DrawRectangleRounded(shadow, 0.035f, 8, Fade(ink, 0.12f));
    DrawRectangleRounded(area, 0.035f, 8, wood_dark);
    DrawRectangleRounded(inset(area, 1), 0.035f, 8, wood);
    for (int i = 0; i < 31; ++i) {
        const float y = area.y + 12 + (area.height - 24) * (float)i / 30.0f;
        const float x = area.x + 9 + (float)((i * 11) % 19);
        const float end = area.x + area.width - 9 - (float)((i * 7) % 17);
        const Color grain = i % 3 == 0 ? Fade(ivory, 0.16f) : Fade(wood_dark, 0.09f);
        horizontal_line(x, end, y, grain);
    }
}

static void draw_grid(Rectangle area, int size, float padding)
{
    for (int i = 0; i < size; ++i) {
        const float weight = (i == 0 || i == size - 1) ? 1.7f : 1.0f;
        const Color stroke = Fade(wood_dark, 0.8f);
        DrawLineEx(intersection(area, size, padding, (Move){ i, 0 }),
                   intersection(area, size, padding, (Move){ i, size - 1 }), weight, stroke);
        DrawLineEx(intersection(area, size, padding, (Move){ 0, i }),
                   intersection(area, size, padding, (Move){ size - 1, i }), weight, stroke);
    }
}

static void draw_stone(Vector2 point, float radius, Stone stone, float opacity)
{
    const Color shadow = { 46, 32, 23, 255 };
    const Color rim = stone == BLACK ? (Color){ 31, 36, 33, 255 } : (Color){ 154, 139, 115, 255 };
    const Color center = stone == BLACK ? (Color){ 74, 82, 77, 255 } : (Color){ 255, 253, 242, 255 };
    const Color outer = stone == BLACK ? (Color){ 34, 40, 36, 255 } : (Color){ 220, 214, 194, 255 };
    DrawCircleV((Vector2){ point.x + 1, point.y + 2 }, radius, Fade(shadow, 0.25f * opacity));
    DrawCircleV(point, radius, Fade(rim, opacity));
    DrawCircleGradient((int)point.x, (int)point.y, radius - 1, Fade(center, opacity), Fade(outer, opacity));
}

static void draw_menu(Font font, const UiView *view)
{
    const Rectangle hero = { 48, 136, 590, 584 };
    const Rectangle mini = { 310, 366, 276, 276 };
    card(hero, paper);
    DrawRectangleRounded((Rectangle){ 76, 177, 41, 5 }, 1, 6, red);
    label(font, T_MENU_TITLE, 76, 199, 72, ink);
    label(font, T_MENU_SUBTITLE, 79, 290, 25, muted);
    draw_wood(mini);
    draw_grid(mini, 9, 24);
    static const Move motif[] = { { 5, 3 }, { 3, 4 }, { 4, 4 }, { 4, 5 },
                                  { 3, 5 }, { 3, 6 }, { 2, 6 }, { 5, 5 } };
    for (unsigned int i = 0; i < sizeof motif / sizeof motif[0]; ++i) {
        draw_stone(intersection(mini, 9, 24, motif[i]), 12, i % 2 == 0 ? BLACK : WHITE, 1);
    }
    const Vector2 seal = intersection(mini, 9, 24, motif[7]);
    DrawPoly(seal, 4, 4, 0, red);
    label(font, T_MENU_BOARD, 79, 558, 36, forest);
    label(font, T_MENU_RULE, 79, 608, 18, muted);

    const Rectangle panel = { 680, 136, 372, 584 };
    card(panel, paper);
    label(font, T_MENU_PICK, 708, 178, 38, ink);
    label(font, T_MENU_PICK_NOTE, 709, 237, 21, muted);
    horizontal_line(708, 1024, 278, outline);
    horizontal_line(708, 1024, 616, outline);
    draw_stone((Vector2){ 720, 660 }, 10, view->player, 1);
    label(font, stone_name(view->player), 741, 648, 23, ink);
    label(font, difficulty_name(view->difficulty), 970, 648, 23, forest);
}

static void draw_settings(Font font)
{
    card(page_panel, paper);
    label(font, T_SETTINGS, 224, 181, 44, ink);
    label(font, T_SETTINGS_NOTE, 224, 242, 23, muted);
    horizontal_line(224, 876, 279, outline);
    label(font, T_SELECT_STONE, 224, 299, 24, forest);
    label(font, T_SELECT_DIFFICULTY, 224, 423, 24, forest);
    horizontal_line(224, 876, 563, outline);
}

static void draw_rules(Font font)
{
    card(page_panel, paper);
    label(font, T_RULES, 224, 181, 44, ink);
    label(font, T_MENU_BOARD, 224, 242, 23, muted);
    horizontal_line(224, 876, 279, outline);

    static const char *const rules[] = { T_RULE_TURN, T_RULE_WIN, T_RULE_FREE, T_RULE_DRAW };
    for (int i = 0; i < 4; ++i) {
        const float y = 299 + (float)i * 65;
        const Rectangle number = { 224, y, 39, 39 };
        DrawRectangleRounded(number, 0.28f, 8, sage);
        center_label(font, TextFormat("%d", i + 1), number, 21, forest);
        label(font, rules[i], 283, y + 8, 23, ink);
        horizontal_line(224, 876, y + 52, outline);
    }
}

static void draw_focus_at(Vector2 point)
{
    for (int x = -1; x <= 1; x += 2) {
        for (int y = -1; y <= 1; y += 2) {
            const Vector2 corner = { point.x + 16.0f * (float)x, point.y + 16.0f * (float)y };
            const Vector2 horizontal = { point.x + 9.0f * (float)x, corner.y };
            const Vector2 vertical = { corner.x, point.y + 9.0f * (float)y };
            DrawLineEx(corner, horizontal, 4, ivory);
            DrawLineEx(corner, vertical, 4, ivory);
            DrawLineEx(corner, horizontal, 2, forest);
            DrawLineEx(corner, vertical, 2, forest);
        }
    }
}

static void draw_board(Font font, const Ui *ui, const Game *game, Vector2 mouse, bool modal)
{
    const float padding = 38;
    draw_wood(board_area);
    draw_grid(board_area, BOARD_SIZE, padding);

    for (int i = 0; i < BOARD_SIZE; ++i) {
        const Vector2 top = intersection(board_area, BOARD_SIZE, padding, (Move){ 0, i });
        const Vector2 left = intersection(board_area, BOARD_SIZE, padding, (Move){ i, 0 });
        const char column[] = { (char)('A' + i), '\0' };
        center_label(font, column, (Rectangle){ top.x - 10, top.y - 31, 20, 19 }, 14, ink);
        center_label(font, TextFormat("%d", i + 1),
                     (Rectangle){ left.x - 35, left.y - 9, 25, 18 }, 14, ink);
    }

    static const Move stars[] = { { 3, 3 }, { 3, 11 }, { 7, 7 }, { 11, 3 }, { 11, 11 } };
    for (unsigned int i = 0; i < sizeof stars / sizeof stars[0]; ++i) {
        DrawCircleV(intersection(board_area, BOARD_SIZE, padding, stars[i]), 3.8f, wood_dark);
    }

    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            if (game->cells[row][col] != EMPTY) {
                draw_stone(intersection(board_area, BOARD_SIZE, padding, (Move){ row, col }),
                           14.5f, game->cells[row][col], 1);
            }
        }
    }

    if (game->winner != EMPTY) {
        const Vector2 first = intersection(board_area, BOARD_SIZE, padding, game->win_start);
        const Vector2 last = intersection(board_area, BOARD_SIZE, padding, game->win_end);
        DrawLineEx(first, last, 7, ivory);
        DrawLineEx(first, last, 3, red);
        DrawCircleV(first, 5.5f, ivory);
        DrawCircleV(last, 5.5f, ivory);
        DrawCircleV(first, 3.5f, red);
        DrawCircleV(last, 3.5f, red);
    }

    if (game->count > 0) {
        const Move last = game->history[game->count - 1];
        const Vector2 point = intersection(board_area, BOARD_SIZE, padding, last);
        DrawPoly(point, 4, 7, 0, ivory);
        DrawPoly(point, 4, 4.5f, 0, red);
    }

    if (!modal && !game_over(game)) {
        const Move hover = board_move_at(mouse);
        if (player_can_place(game, hover)) {
            draw_stone(intersection(board_area, BOARD_SIZE, padding, hover), 14.5f, game->player, 0.55f);
        }
        if (ui->keyboard && ui->focus == 0) {
            draw_focus_at(intersection(board_area, BOARD_SIZE, padding, ui->board_focus));
        }
    }
}

static void draw_match(Font font, const Ui *ui, const UiView *view, Vector2 mouse)
{
    const Game *game = view->game;
    const bool finished = game_over(game);
    const bool thinking = !finished && game->turn != game->player;
    draw_board(font, ui, game, mouse, view->confirmation != UI_CONFIRM_NONE);
    card(match_panel, paper);

    const Rectangle status = { 680, 160, 348, 151 };
    DrawRectangleRounded(status, 0.15f, 8, sage);
    label(font, T_STATUS_LABEL, 700, 174, 18, muted);
    const char *status_text = T_YOUR_TURN;
    const char *status_note = T_YOUR_TURN_NOTE;
    if (finished) {
        status_text = game->draw ? T_DRAW_GAME : game->winner == game->player ? T_YOU_WIN : T_COMPUTER_WINS;
        status_note = T_FINISHED_NOTE;
    } else if (thinking) {
        status_text = T_THINKING;
        status_note = T_THINKING_NOTE;
    }
    label(font, status_text, 700, 207, 34, forest);
    label(font, status_note, 700, 263, 19, muted);

    draw_stone((Vector2){ 700, 344 }, 9, game->player, 1);
    label(font, T_YOU, 723, 331, 24, ink);
    label(font, stone_name(game->player), 966, 331, 24, muted);
    draw_stone((Vector2){ 700, 389 }, 9, game_other(game->player), 1);
    label(font, T_COMPUTER, 723, 376, 24, ink);
    label(font, stone_name(game_other(game->player)), 966, 376, 24, muted);
    label(font, T_DIFFICULTY_LABEL, 723, 421, 24, ink);
    label(font, difficulty_name(view->difficulty), 966, 421, 24, forest);
    horizontal_line(684, 1024, 461, outline);

    DrawPoly((Vector2){ 939, 761 }, 4, 5, 0, red);
    label(font, T_LAST_MOVE, 954, 751, 18, muted);
}

static const char *confirmation_title(UiConfirmation confirmation)
{
    if (confirmation == UI_CONFIRM_RESTART) return T_CONFIRM_RESTART;
    if (confirmation == UI_CONFIRM_MENU) return T_CONFIRM_MENU;
    return T_CONFIRM_EXIT;
}

static void draw_buttons(Font font, const UiView *view, Vector2 mouse, int focus, bool keyboard)
{
    Button buttons[6];
    const int count = build_buttons(view, buttons);
    for (int i = 0; i < count; ++i) draw_button(font, &buttons[i], mouse, keyboard && i == focus);
}

void ui_draw(const Ui *ui, const UiView *view, Font font)
{
    const Camera2D camera = viewport();
    const Vector2 mouse = GetScreenToWorld2D(GetMousePosition(), camera);
    const bool modal = view->confirmation != UI_CONFIRM_NONE;

    ClearBackground(background);
    BeginMode2D(camera);
    draw_frame(font, view->screen == UI_MATCH);
    switch (view->screen) {
        case UI_MENU: draw_menu(font, view); break;
        case UI_SETTINGS: draw_settings(font); break;
        case UI_RULES: draw_rules(font); break;
        case UI_MATCH: draw_match(font, ui, view, mouse); break;
    }
    UiView behind = *view;
    behind.confirmation = UI_CONFIRM_NONE;
    draw_buttons(font, &behind, modal ? (Vector2){ -1, -1 } : mouse,
                 ui->focus, ui->keyboard && !modal);
    EndMode2D();

    if (modal) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(ink, 0.57f));
        BeginMode2D(camera);
        card(dialog, paper);
        label(font, confirmation_title(view->confirmation), 340, 314, 34, ink);
        label(font, T_CONFIRM_LOSS, 340, 371, 23, muted);
        draw_buttons(font, view, mouse, ui->modal_open ? ui->modal_focus : 0, true);
        EndMode2D();
    }
}

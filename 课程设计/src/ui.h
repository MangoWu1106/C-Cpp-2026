#ifndef GOBANG_UI_H
#define GOBANG_UI_H

#include <raylib.h>

/* The game uses these names for stone values. */
#undef BLACK
#undef WHITE

#include "ai.h"

typedef enum { UI_MENU, UI_SETTINGS, UI_RULES, UI_MATCH } UiScreen;

typedef enum {
    UI_CONFIRM_NONE, UI_CONFIRM_RESTART, UI_CONFIRM_MENU, UI_CONFIRM_EXIT
} UiConfirmation;

typedef enum {
    UI_NONE, UI_START, UI_OPEN_SETTINGS, UI_OPEN_RULES, UI_EXIT, UI_BACK,
    UI_SET_STONE, UI_SET_DIFFICULTY, UI_PLACE, UI_UNDO, UI_RESTART,
    UI_RETURN_MENU, UI_CONFIRM, UI_CANCEL
} UiCommand;

typedef struct {
    UiScreen screen;
    UiConfirmation confirmation;
    const Game *game;
    Stone player;
    Difficulty difficulty;
} UiView;

typedef struct {
    UiCommand command;
    Move move;
    Stone player;
    Difficulty difficulty;
} UiAction;

typedef struct {
    Move board_focus;
    int focus;
    int modal_focus;
    bool initialized;
    bool keyboard;
    bool modal_open;
} Ui;

UiAction ui_update(Ui *ui, const UiView *view);
void ui_draw(const Ui *ui, const UiView *view, Font font);
const char *ui_font_text(void);

#endif

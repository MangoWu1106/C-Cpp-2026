#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <termios.h>
#endif

static int terminal_input;
#ifdef _WIN32
static HANDLE input_handle;
static DWORD original_input_mode;

static void restore_terminal(void)
{
    if (terminal_input) SetConsoleMode(input_handle, original_input_mode);
}
#else
static struct termios original_terminal;

static void restore_terminal(void)
{
    tcsetattr(STDIN_FILENO, TCSANOW, &original_terminal);
}
#endif

static void initialize_input(void)
{
#ifdef _WIN32
    input_handle = GetStdHandle(STD_INPUT_HANDLE);
    if (GetConsoleMode(input_handle, &original_input_mode)) {
        DWORD mode = original_input_mode &
            ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT);
        if (atexit(restore_terminal) == 0 && SetConsoleMode(input_handle, mode))
            terminal_input = 1;
    }
#else
    if (isatty(STDIN_FILENO) &&
        tcgetattr(STDIN_FILENO, &original_terminal) == 0) {
        struct termios mode = original_terminal;
        mode.c_lflag &= ~(ICANON | ECHO | ISIG);
        mode.c_cc[VMIN] = 1;
        mode.c_cc[VTIME] = 0;
        if (atexit(restore_terminal) == 0 &&
            tcsetattr(STDIN_FILENO, TCSANOW, &mode) == 0)
            terminal_input = 1;
    }
#endif
    if (terminal_input)
        puts("Press keys directly; Enter is not needed.");
    else
        puts("Input is supplied by the IDE or a pipe.\n"
             "If keys do not arrive immediately, press Enter or use a terminal.");
}

/* ANSI terminals repaint the same screen instead of adding another board. */
static int ansi_output;
#ifdef _WIN32
static HANDLE output_handle;
static int native_output;
#endif

static void initialize_display(void)
{
#ifdef _WIN32
    DWORD mode;
    output_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    native_output = GetConsoleMode(output_handle, &mode) != 0;
    if (native_output)
        ansi_output = SetConsoleMode(output_handle,
            mode | ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
    else {
        const char *term = getenv("TERM");
        ansi_output = term && strcmp(term, "dumb") != 0;
    }
#else
    const char *term = getenv("TERM");
    ansi_output = isatty(STDOUT_FILENO) && (!term || strcmp(term, "dumb") != 0);
#endif
}

static void clear_screen(void)
{
    if (ansi_output) {
        fputs("\x1b[2J\x1b[H", stdout);
    }
#ifdef _WIN32
    else if (native_output) {
        CONSOLE_SCREEN_BUFFER_INFO info;
        DWORD written;
        COORD home = {0, 0};
        if (GetConsoleScreenBufferInfo(output_handle, &info)) {
            DWORD cells = (DWORD)info.dwSize.X * (DWORD)info.dwSize.Y;
            FillConsoleOutputCharacterA(output_handle, ' ', cells, home, &written);
            FillConsoleOutputAttribute(output_handle, info.wAttributes, cells, home, &written);
            SetConsoleCursorPosition(output_handle, home);
        }
    }
#endif
}

static void end_display_line(void)
{
    if (ansi_output) fputs("\x1b[K", stdout);
    putchar('\n');
}

#ifdef GAME_DATA_DIR
#undef GAME_DATA_DIR
#endif
static char game_data_dir[900];
#define GAME_DATA_DIR game_data_dir

/* Search beside the executable, the working directory, and the source file.
   Also support the surrounding CLion project and its build directories. */
static int find_game_data_dir(const char *executable)
{
    const char *roots[] = {executable, ".", __FILE__};
    const char *folders[] = {"", "/level1/p06_push_boxes"};
    for (int i = 0; i < 3; ++i) {
        char base[900];
        if (!roots[i] || strlen(roots[i]) >= sizeof base) continue;
        strcpy(base, roots[i]);
        if (i != 1) {
            char *slash = strrchr(base, '/');
            char *backslash = strrchr(base, '\\');
            if (backslash && (!slash || backslash > slash)) slash = backslash;
            if (!slash) continue;
            slash[1] = '\0';
        }
        for (int depth = 0; depth < 5; ++depth) {
            for (int j = 0; j < 2; ++j) {
                char candidate[900], path[1024];
                int n = snprintf(candidate, sizeof candidate, "%s%s", base, folders[j]);
                if (n < 0 || (size_t)n >= sizeof candidate) continue;
                int level;
                for (level = 1; level <= 5; ++level) {
                    snprintf(path, sizeof path, "%s/levels/level%d.txt", candidate, level);
                    FILE *f = fopen(path, "r");
                    if (!f) break;
                    fclose(f);
                }
                if (level == 6) {
                    strcpy(game_data_dir, candidate);
                    return 1;
                }
            }
            if (strlen(base) + 3 >= sizeof base) break;
            strcat(base, "/..");
        }
    }
    puts("Cannot find all five level files. Keep the levels folder beside the executable,");
    puts("or keep the project structure level1/p06_push_boxes/levels.");
    return 0;
}

#define MAX_ROWS 20
#define MAX_COLS 40
#define LEVEL_COUNT 5

typedef struct {
    int rows, cols, player_row, player_col;
    char floor[MAX_ROWS][MAX_COLS];
    char box[MAX_ROWS][MAX_COLS];
    unsigned long moves;
} Game;

/* Read a rectangular map, with or without a rows/columns header.
   Saved score lines below the map are not part of the board. */
static int load_level(Game *g, int level)
{
    char path[1024], line[256];
    char layout[MAX_ROWS][MAX_COLS];
    int players = 0, boxes[3] = {0}, goals[3] = {0};
    snprintf(path, sizeof path, "%s/levels/level%d.txt", GAME_DATA_DIR, level);
    FILE *f = fopen(path, "r");
    if (!f) {
        printf("Cannot open %s\n", path);
        return 0;
    }
    memset(g, 0, sizeof *g);
    do {
        if (!fgets(line, sizeof line, f)) goto invalid;
        line[strcspn(line, "\r\n")] = '\0';
    } while (line[0] == '\0');

    int expected_rows = 0, expected_cols = 0;
    char extra;
    if (sscanf(line, "%d %d %c", &expected_rows, &expected_cols, &extra) == 2) {
        if (expected_rows < 3 || expected_rows > MAX_ROWS ||
            expected_cols < 3 || expected_cols > MAX_COLS) goto invalid;
        if (!fgets(line, sizeof line, f)) goto invalid;
    } else {
        expected_rows = expected_cols = 0;
    }

    do {
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] != '#') break;
        size_t width = strlen(line);
        if (width < 3 || width > MAX_COLS || g->rows >= MAX_ROWS) goto invalid;
        if (g->rows == 0) g->cols = (int)width;
        if (width != (size_t)g->cols) goto invalid;
        memcpy(layout[g->rows++], line, width);
    } while (fgets(line, sizeof line, f));
    if (ferror(f) || g->rows < 3 ||
        (expected_rows && (g->rows != expected_rows || g->cols != expected_cols)))
        goto invalid;

    for (int r = 0; r < g->rows; ++r) {
        for (int c = 0; c < g->cols; ++c) {
            char ch = layout[r][c];
            g->floor[r][c] = '.';
            if ((r == 0 || c == 0 || r == g->rows - 1 ||
                 c == g->cols - 1) && ch != '#') goto invalid;
            if (ch == '#' || ch == '.' || (ch >= 'a' && ch <= 'c')) {
                g->floor[r][c] = ch;
                if (ch >= 'a' && ch <= 'c') ++goals[ch - 'a'];
            } else if (ch >= 'A' && ch <= 'C') {
                g->box[r][c] = ch;
                ++boxes[ch - 'A'];
            } else if (ch == '@') {
                g->player_row = r;
                g->player_col = c;
                ++players;
            } else goto invalid;
        }
    }
    if (players != 1 || boxes[0] + boxes[1] + boxes[2] == 0) goto invalid;
    for (int i = 0; i < 3; ++i)
        if (boxes[i] != goals[i] || boxes[i] > 1) goto invalid;
    fclose(f);
    return 1;

invalid:
    printf("Invalid level file: %s\n", path);
    fclose(f);
    return 0;
}

static unsigned long points(const Game *g)
{
    return g->moves >= 100 ? 0 : 1000 - 10 * g->moves;
}

static int inside(const Game *g, int r, int c)
{
    return r >= 0 && r < g->rows && c >= 0 && c < g->cols;
}

/* Return 1 only if the player actually moves. Pushing counts as one move. */
static int move_player(Game *g, int dr, int dc)
{
    int r = g->player_row + dr, c = g->player_col + dc;
    if (!inside(g, r, c) || g->floor[r][c] == '#') return 0;
    if (g->box[r][c]) {
        int nr = r + dr, nc = c + dc;
        if (!inside(g, nr, nc) || g->floor[nr][nc] == '#' ||
            g->box[nr][nc]) return 0;
        g->box[nr][nc] = g->box[r][c];
        g->box[r][c] = 0;
    }
    g->player_row = r;
    g->player_col = c;
    ++g->moves;
    return 1;
}

static int won(const Game *g)
{
    for (int r = 0; r < g->rows; ++r)
        for (int c = 0; c < g->cols; ++c)
            if (g->box[r][c] &&
                g->floor[r][c] != (char)tolower((unsigned char)g->box[r][c]))
                return 0;
    return 1;
}

static void draw(const Game *g, int level, const char *message)
{
    if (ansi_output) fputs("\x1b[H", stdout);
#ifdef _WIN32
    else if (native_output) clear_screen();
#endif
    printf("Level %d | Moves: %lu | Points: %lu", level, g->moves, points(g));
    end_display_line();
    for (int r = 0; r < g->rows; ++r) {
        for (int c = 0; c < g->cols; ++c) {
            char ch = g->floor[r][c];
            if (g->box[r][c]) ch = g->box[r][c];
            if (r == g->player_row && c == g->player_col) ch = '@';
            putchar(ch);
        }
        end_display_line();
    }
    fputs("@ player | # wall | . floor | A->a, B->b, C->c", stdout);
    end_display_line();
    fputs("W/A/S/D move | R restart | Q menu | Use English input", stdout);
    end_display_line();
    fputs(message, stdout);
    end_display_line();
    if (ansi_output) fputs("\x1b[J", stdout);
}

static int save_score(const Game *g, int level)
{
    char path[1024];
    snprintf(path, sizeof path, "%s/levels/level%d.txt", GAME_DATA_DIR, level);
    /* Update the existing level only. Never create a separate score file.
       Binary mode preserves the map bytes and allows checking its last byte. */
    FILE *f = fopen(path, "r+b");
    if (!f) {
        printf("Could not open level file for saving: %s\n", path);
        return 0;
    }
    int ok = fseek(f, -1L, SEEK_END) == 0;
    int last = ok ? fgetc(f) : EOF;
    if (last == EOF || fseek(f, 0, SEEK_END) != 0) ok = 0;
    if (ok && last != '\n' && fputc('\n', f) == EOF) ok = 0;
    if (ok && fprintf(f, "Moves: %lu Points: %lu\n", g->moves, points(g)) < 0)
        ok = 0;
    if (fclose(f) != 0) ok = 0;
    if (ok) printf("Result saved to %s\n", path);
    else puts("Could not finish saving the result.");
    return ok;
}

/* Read every command, including multiple letters entered in an IDE console. */
static int read_command(void)
{
    for (;;) {
        int ch;
#ifdef _WIN32
        /* Read the actual standard input handle, including IDE/PTY pipes. */
        unsigned char byte;
        DWORD count;
        if (!ReadFile(input_handle, &byte, 1, &count, NULL) || count == 0)
            return EOF;
        ch = byte;
#else
        ch = getchar();
#endif
        if (ch == EOF || ch == 3 || ch == 4 || ch == 26) return EOF;
        if (isspace((unsigned char)ch)) continue;
        return tolower((unsigned char)ch);
    }
}

int main(int argc, char *argv[])
{
    if (!find_game_data_dir(argc > 0 ? argv[0] : NULL)) return 1;
    setvbuf(stdout, NULL, _IONBF, 0);
    initialize_input();
    initialize_display();
    puts("MATCH THE BOXES\nPush every box onto its matching lowercase target.");
    puts("You can push one box at a time; you cannot pull boxes.");
    puts("Points = max(0, 1000 - 10 * successful moves).");
    for (;;) {
        puts("\nChoose level 1-5, or Q to quit:");
        int command = read_command();
        if (command == EOF || command == 'q') break;
        if (command < '1' || command > '5') continue;
        int level = command - '0';
        Game game;
        if (!load_level(&game, level)) continue;
        const char *message = "";
        clear_screen();
        for (;;) {
            draw(&game, level, message);
            if (won(&game)) {
                printf("YOU WIN! Moves: %lu | Points: %lu\n", game.moves, points(&game));
                save_score(&game, level);
                break;
            }
            command = read_command();
            if (command == EOF) return 0;
            if (command == 'q') break;
            if (command == 'r') {
                if (!load_level(&game, level)) break;
                message = "Level restarted.";
                continue;
            }
            int moved = 0;
            switch (command) {
                case 'w': moved = move_player(&game, -1, 0); break;
                case 's': moved = move_player(&game, 1, 0); break;
                case 'a': moved = move_player(&game, 0, -1); break;
                case 'd': moved = move_player(&game, 0, 1); break;
                default: message = "Use W, A, S, D, R or Q (English input)."; continue;
            }
            message = moved ? "" : "Blocked! Move not counted.";
        }
    }
    return 0;
}

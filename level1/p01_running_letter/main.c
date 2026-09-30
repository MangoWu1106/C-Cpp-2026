#include <stdio.h>
#include <windows.h>
#include <conio.h>

int main(void)
{
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD old_mode;
    CONSOLE_CURSOR_INFO old_cursor;

    if (!GetConsoleMode(console, &old_mode) ||
        !GetConsoleCursorInfo(console, &old_cursor))
    {
        fprintf(stderr, "Run this program in a Windows terminal.\n");
        return 1;
    }

    /* Enable terminal commands and disable right-edge wrapping. */
    DWORD mode = (old_mode | ENABLE_PROCESSED_OUTPUT |
                 ENABLE_VIRTUAL_TERMINAL_PROCESSING)
                 & ~ENABLE_WRAP_AT_EOL_OUTPUT;

    if (!SetConsoleMode(console, mode))
    {
        fprintf(stderr,
                "This console does not support terminal commands.\n");
        return 1;
    }

    /* Enter a temporary screen and hide the cursor. */
    fputs("\x1b[?1049h\x1b[?25l", stdout);
    fflush(stdout);

    int x = 0;
    int direction = 1;
    int failed = 0;

    /* Press any character key to stop. */
    while (!_kbhit())
    {
        CONSOLE_SCREEN_BUFFER_INFO info;

        if (!GetConsoleScreenBufferInfo(console, &info))
        {
            failed = 1;
            break;
        }

        int width = info.srWindow.Right - info.srWindow.Left + 1;

        /* Keep the position valid after resizing. */
        if (x >= width)
            x = width - 1;

        if (x < 0)
            x = 0;

        /*
         * Clear the screen, position the cursor, and draw one @.
         * Terminal row and column numbers start at 1.
         * Move the hidden cursor home afterward.
         */
        if (printf("\x1b[2J\x1b[1;%dH@\x1b[H", x + 1) < 0 ||
            fflush(stdout) == EOF)
        {
            failed = 1;
            break;
        }

        Sleep(80);

        if (width > 1)
        {
            if (x >= width - 1)
                direction = -1;
            else if (x <= 0)
                direction = 1;

            x += direction;
        }
    }

    if (!failed)
        _getch();

    /* Restore the original screen and console settings. */
    fputs("\x1b[?1049l", stdout);
    fflush(stdout);

    SetConsoleCursorInfo(console, &old_cursor);
    SetConsoleMode(console, old_mode);

    if (failed)
        fprintf(stderr, "The terminal could not be read or updated.\n");

    return failed;
}
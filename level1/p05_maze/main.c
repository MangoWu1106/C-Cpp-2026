#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <conio.h>
#include <windows.h>

/* Keep both dimensions odd and at least 5. */
#define HEIGHT 21
#define WIDTH  41

char maze[HEIGHT][WIDTH];

void initializeTerminal(void)
{
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;

    /* Enable ANSI sequences in a native Windows console.
       Terminal emulators may process them directly. */
    if (GetConsoleMode(output, &mode)) {
        SetConsoleMode(
            output,
            mode | ENABLE_PROCESSED_OUTPUT |
                   ENABLE_VIRTUAL_TERMINAL_PROCESSING
        );
    }

    /* Clear the screen once and move to the top-left. */
    printf("\x1b[2J\x1b[H");
    fflush(stdout);
}

/* Create a random maze using recursive backtracking. */
void carveMaze(int row, int col)
{
    int directions[4] = {0, 1, 2, 3};
    const int dr[4] = {-2, 2, 0, 0};
    const int dc[4] = {0, 0, -2, 2};
    int i;

    maze[row][col] = ' ';

    /* Shuffle the directions. */
    for (i = 3; i > 0; --i) {
        int j = rand() % (i + 1);
        int temp = directions[i];
        directions[i] = directions[j];
        directions[j] = temp;
    }

    for (i = 0; i < 4; ++i) {
        int direction = directions[i];
        int nextRow = row + dr[direction];
        int nextCol = col + dc[direction];

        if (nextRow > 0 && nextRow < HEIGHT - 1 &&
            nextCol > 0 && nextCol < WIDTH - 1 &&
            maze[nextRow][nextCol] == '#') {

            /* Remove the wall between the two cells. */
            maze[row + dr[direction] / 2]
                [col + dc[direction] / 2] = ' ';

            carveMaze(nextRow, nextCol);
        }
    }
}

void generateMaze(void)
{
    int row, col;

    for (row = 0; row < HEIGHT; ++row) {
        for (col = 0; col < WIDTH; ++col) {
            maze[row][col] = '#';
        }
    }

    carveMaze(1, 1);
    maze[HEIGHT - 2][WIDTH - 2] = 'E';
}

void displayMaze(int playerRow, int playerCol)
{
    int row, col;

    /* Position each line explicitly instead of printing newlines. */
    printf("\x1b[1;1HRANDOM MAZE GAME\x1b[K");
    printf("\x1b[2;1HArrow keys: move | Q: quit\x1b[K");
    printf("\x1b[3;1H# = Wall | @ = Player | E = Exit\x1b[K");
    printf("\x1b[4;1H\x1b[K");

    for (row = 0; row < HEIGHT; ++row) {
        /* ANSI positions start at 1. Maze starts on screen row 5. */
        printf("\x1b[%d;1H", row + 5);

        for (col = 0; col < WIDTH; ++col) {
            if (row == playerRow && col == playerCol) {
                putchar('@');
            } else {
                putchar(maze[row][col]);
            }
        }

        /* Clear anything remaining to the right of this row. */
        printf("\x1b[K");
    }

    /* Leave the cursor below the maze for the result message. */
    printf("\x1b[%d;1H", HEIGHT + 5);
    fflush(stdout);
}

int main(void)
{
    int playerRow = 1;
    int playerCol = 1;
    LARGE_INTEGER counter;

    /* Use a high-resolution clock so quick restarts vary the seed. */
    QueryPerformanceCounter(&counter);
    srand((unsigned int)counter.QuadPart ^
          (unsigned int)time(NULL) ^
          (unsigned int)GetCurrentProcessId());

    initializeTerminal();
    generateMaze();

    while (1) {
        int key;
        int nextRow = playerRow;
        int nextCol = playerCol;

        displayMaze(playerRow, playerCol);

        if (maze[playerRow][playerCol] == 'E') {
            printf("\nCongratulations! You win the game!\n");
            break;
        }

        /* Read a key immediately, without pressing Enter. */
        key = _getch();

        if (key == 'q' || key == 'Q') {
            printf("\nGame ended.\n");
            break;
        }

        /* Arrow keys have a prefix and a direction code. */
        if (key == 0 || key == 224) {
            key = _getch();

            switch (key) {
                case 72: --nextRow; break; /* Up */
                case 80: ++nextRow; break; /* Down */
                case 75: --nextCol; break; /* Left */
                case 77: ++nextCol; break; /* Right */
                default: break;
            }
        }

        if (nextRow >= 0 && nextRow < HEIGHT &&
            nextCol >= 0 && nextCol < WIDTH &&
            maze[nextRow][nextCol] != '#') {
            playerRow = nextRow;
            playerCol = nextCol;
        }
    }

    printf("Press any key to close...");
    fflush(stdout);
    _getch();

    return 0;
}
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int is_prime(int number)
{
    if (number < 2) {
        return 0; /* 1 is not prime. */
    }

    /* Division avoids overflow when checking the square-root limit. */
    for (int divisor = 2; divisor <= number / divisor; divisor++) {
        if (number % divisor == 0) {
            return 0;
        }
    }

    return 1;
}

int main(void)
{
    char input[128];
    char *end;
    long value;

    while (1) {
        printf("Enter a positive integer (1 to %d): ", INT_MAX);
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            return 0;
        }

        /* Discard an input line that is too long. */
        if (strchr(input, '\n') == NULL && !feof(stdin)) {
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {
            }
            puts("Invalid input. Please enter a positive integer.");
            continue;
        }

        errno = 0;
        value = strtol(input, &end, 10);

        if (end != input) {
            while (isspace((unsigned char)*end)) {
                end++;
            }
            if (errno != ERANGE && *end == '\0' &&
                value > 0 && value <= INT_MAX) {
                break;
                }
        }

        puts("Invalid input. Please enter a positive integer.");
    }

    if (is_prime((int)value)) {
        printf("%ld is a prime number.\n", value);
    } else {
        printf("%ld is not a prime number.\n", value);
    }

    return 0;
}
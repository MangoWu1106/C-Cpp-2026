#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdio.h>
#include <stddef.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

enum { LIMIT = 1000 };

/* Convert without a separate printf call for every prime. */
static char *append_number(char *destination, unsigned int number)
{
    char digits[10];
    unsigned int count = 0;
    do {
        digits[count++] = (char)('0' + number % 10);
        number /= 10;
    } while (number != 0);

    do {
        *destination++ = digits[--count];
    } while (count != 0);
    return destination;
}

int main(void)
{
    /* Start timing before initializing the sieve or preparing output. */
#ifdef _WIN32
    LARGE_INTEGER start, end, frequency;
    QueryPerformanceCounter(&start);
#else
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
#endif

    /* Index n / 2 represents odd number n; even numbers need no storage. */
    unsigned char composite[LIMIT / 2] = {0};
    for (unsigned int p = 3; p * p <= LIMIT; p += 2) {
        if (!composite[p / 2]) {
            /* Smaller multiples already have a smaller prime factor. */
            for (unsigned int multiple = p * p; multiple <= LIMIT;
                 multiple += 2 * p) {
                composite[multiple / 2] = 1;
            }
        }
    }

    /* For this fixed range, each odd candidate needs at most four bytes
       (a space and three digits). Allow space for 2 and the newline too. */
    char output[4 * (LIMIT / 2) + 2];
    char *next = output;
    *next++ = '2';
    for (unsigned int n = 3; n <= LIMIT; n += 2) {
        if (!composite[n / 2]) {
            *next++ = ' ';
            next = append_number(next, n);
        }
    }
    *next++ = '\n';

    const size_t length = (size_t)(next - output);
    if (fwrite(output, 1, length, stdout) != length || fflush(stdout) != 0) {
        fputs("Could not write prime numbers.\n", stderr);
        return 1;
    }

    /* Include calculation, formatting, writing, and flushing the primes.
       The timing report itself is necessarily outside the measurement. */
#ifdef _WIN32
    QueryPerformanceCounter(&end);
    QueryPerformanceFrequency(&frequency);
    const double elapsed = (double)(end.QuadPart - start.QuadPart)
                         / (double)frequency.QuadPart;
#else
    clock_gettime(CLOCK_MONOTONIC, &end);
    const double elapsed = (double)(end.tv_sec - start.tv_sec)
                         + (double)(end.tv_nsec - start.tv_nsec) / 1e9;
#endif
    printf("Time to calculate and print: %.3f microseconds (%.9f seconds)\n",
           elapsed * 1e6, elapsed);
    return 0;
}

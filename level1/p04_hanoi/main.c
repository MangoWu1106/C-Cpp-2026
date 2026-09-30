#include <stdio.h>

void hanoi(int n, char source, char helper, char destination) {
    if (n == 0) {
        return;
    }

    hanoi(n - 1, source, destination, helper);
    printf("%c -> %c\n", source, destination);
    hanoi(n - 1, helper, source, destination);
}

int main(void) {
    hanoi(4, 'A', 'B', 'C');
    return 0;
}
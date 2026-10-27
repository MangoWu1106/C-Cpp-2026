#include <stdio.h>
#include <stdlib.h>

typedef struct Node {
    int value;
    struct Node *next;
} Node;

void print_list(const Node *head) {
    for (const Node *current = head; current != NULL; current = current->next) {
        printf("%d%s", current->value, current->next != NULL ? " -> " : "\n");
    }
}

Node *reverse_list(Node *head) {
    Node *previous = NULL;
    Node *current = head;

    while (current != NULL) {
        Node *next = current->next;  /* Save the next node before changing the link. */
        current->next = previous;
        previous = current;
        current = next;
    }

    return previous;  /* The old tail is now the head. */
}

/* Positions start at 1. Pass 0 to search from the beginning. */
int find_next(const Node *head, int value, int after_position) {
    int position = 1;

    for (const Node *current = head; current != NULL; current = current->next) {
        if (position > after_position && current->value == value) {
            return position;
        }
        ++position;
    }

    return -1;
}

void free_list(Node *head) {
    while (head != NULL) {
        Node *next = head->next;
        free(head);
        head = next;
    }
}

int main(void) {
    int values[] = {1, 5, 3, 4, 5, 6};
    Node *head = NULL;
    Node *tail = NULL;

    /* Create the linked list in main, preserving the array's order. */
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); ++i) {
        Node *node = malloc(sizeof(*node));
        if (node == NULL) {
            fprintf(stderr, "Failed to allocate a node.\n");
            free_list(head);
            return EXIT_FAILURE;
        }

        node->value = values[i];
        node->next = NULL;

        if (head == NULL) {
            head = node;
        } else {
            tail->next = node;
        }
        tail = node;
    }

    printf("Original list: ");
    print_list(head);

    head = reverse_list(head);
    printf("Reversed list: ");
    print_list(head);

    int first = find_next(head, 5, 0);
    printf("First node with value 5 (position): %d\n", first);

    int second = first == -1 ? -1 : find_next(head, 5, first);
    printf("Next node with value 5 (position): %d\n", second);

    free_list(head);
    return EXIT_SUCCESS;
}

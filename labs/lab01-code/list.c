#include "list.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void list_init(list_t *list) {
    list->head = NULL;
    list->size = 0;
}

void list_add(list_t *list, const char *data) {
    node_t *new_node = (node_t *)malloc(sizeof(node_t));

    strncpy(new_node->data, data, MAX_LEN - 1);
    new_node->data[MAX_LEN - 1] = '\0';

    new_node->next = NULL;

    if (list->head == NULL) {
        list->head = new_node;
    } else {
        node_t *curr = list->head;
        while (curr->next != NULL) {
            curr = curr->next;
        }
        curr->next = new_node;
    }

    list->size++;
}

int list_size(const list_t *list) {
    return list -> size;
}

char *list_get(const list_t *list, int index) {
    if (index < 0 || index >= list->size) {
        return NULL;
    }

    node_t *curr = list->head;
    for (int i = 0; i < index; i++) {
        curr = curr -> next;
    }
    return curr -> data;
}

void list_clear(list_t *list) {
    node_t *current = list->head;
    while (current != NULL) {
        node_t *next_node = current->next;
        //free(current->data);
        free(current);
        current = next_node;
    }
    list->head = NULL;
    list->size = 0;
}

int list_contains(const list_t *list, const char *query) {
    node_t *current = list->head;
    while (current != NULL) {
        if (strcmp(current->data, query) == 0) {
            return 1; // Found
        }
        current = current->next;
    }
    return 0; // Not found
}

void list_print(const list_t *list) {
    int i = 0;
    node_t *current = list->head;
    while (current != NULL) {
        printf("%d: %s\n", i, current->data);
        current = current->next;
        i++;
    }
}

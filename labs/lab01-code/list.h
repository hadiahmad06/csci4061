/* List Functions */

#ifndef LIST_H
#define LIST_H
#define MAX_LEN 128

typedef struct node_struct {
    char data[MAX_LEN];
    struct node_struct *next;
} node_t;

typedef struct {
    node_t *head;
    int size;
} list_t;

// Initialize memory for an empty linked list
void list_init(list_t *list);

// Add a new string to the tail of the list
void list_add(list_t *list, const char *data);

// Returns how many nodes are in a list
int list_size(const list_t *list);

// Get a pointer to the data at the specified index. Returns NULL if
// the index is out of bounds.
char *list_get(const list_t *list, int index);

// Remove all nodes associated with the list setting its size to 0
void list_clear(list_t *list);

// Returns 1 if the list contains the given query and 0 otherwise.
int list_contains(const list_t *list, const char *query);

// Print out all nodes in the linked list
void list_print(const list_t *list);

#endif

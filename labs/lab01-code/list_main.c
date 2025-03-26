// Function for a linked list command-line program
#include <stdio.h>
#include <string.h>

#include "list.h"

int main(int argc, char *argv[]) {
    printf("Linked List Demo\n");
    printf("Commands:\n");
    printf("  print:          shows the current contents of the list\n");
    printf("  clear:          eliminates all elements from the list\n");
    printf("  exit:           exit the program\n");
    printf("  insert thing:   inserts the given string into the list\n");
    printf("  size:           prints the size of the list\n");
    printf("  get index:      get the item at the given index\n");
    printf("  contains thing: determine if the given thing is in the list\n");

    char cmd[128];
    list_t list;
    int success;
    list_init(&list);

    while (1) {
        printf("list> ");                      // print prompt
        success = fscanf(stdin, "%s", cmd);    // read command
        if (success == EOF) {                  // check for end of input
            printf("\n");                      // found end of input
            break;                             // break from loop
        }

        if (strcmp("exit", cmd) == 0) {    // check for exit command
            break;                         // break from loop
        }

        else if (strcmp("insert", cmd) == 0) {    // insertion
            fscanf(stdin, "%s", cmd);             // read string to insert
            list_add(&list, cmd);                 // call list_add function
        }

        else if (strcmp("size", cmd) == 0) {    // print size
            printf("%d\n", list_size(&list));
        }

        else if (strcmp("get", cmd) == 0) {    // get command
            int index;
            fscanf(stdin, "%d", &index);    // read an integer

            char *ith = list_get(&list, index);    // call list function
            if (ith == NULL) {                     // check for success
                printf("Out of bounds\n");
            } else {
                printf("%d: %s\n", index, ith);
            }
        }

        else if (strcmp("clear", cmd) == 0) {    // clear command
            list_clear(&list);
        }

        else if (strcmp("print", cmd) == 0) {    // print command
            list_print(&list);
        }

        else if (strcmp("contains", cmd) == 0) {    // contains command
            fscanf(stdin, "%s", cmd);               // Read query
            if (list_contains(&list, cmd)) {
                printf("\'%s\' is present\n", cmd);
            } else {
                printf("Not found\n");
            }
        }

        else {    // unknown command
            printf("Unknown command %s\n", cmd);
        }
    }

    // end main while loop
    list_clear(&list);    // clean up the list
    return 0;
}

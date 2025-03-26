#include <string.h>
#include <stdio.h>

#define BUF_LEN 512

void print_words(char *s) {
    const char *delim = " ";
    char *token = strtok(s, delim);
    while (token) {
        printf("%s\n", token);
        token = strtok(NULL, delim);
    }
}

int main(int argc, char **argv) {
    char sentence[BUF_LEN];

    printf("sentence> ");
    while (fgets(sentence, BUF_LEN, stdin) != NULL) {
        // Remove trailing newline from sentence
        // There are certainly fancier ways to do this
        int i = 0;
        while (sentence[i] != '\n') {
            i++;
        }
        sentence[i] = '\0';

        if (strcmp(sentence, "exit") == 0) {
            return 0;
        }

        print_words(sentence);

        printf("sentence> ");
    }

    printf("\n");
    return 0;
}

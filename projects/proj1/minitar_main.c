#include <stdio.h>
#include <string.h>

#include <stdlib.h>
#include "file_list.h"
#include "minitar.h"

int update_archive(const char *archive_name, const file_list_t *files) {
    FILE *archive = fopen(archive_name, "rb");
    if (!archive) {
        printf("Failed to open archive file: %s", archive_name);
        // perror(err_msg);
        return -1;
    }

    // creates file list for archive
    file_list_t archive_files;
    file_list_init(&archive_files);

    // fetches all files in archive
    get_archive_file_list(archive_name, &archive_files);

    // compares files from archive and files to update
    node_t *archive_file = archive_files.head;
    node_t *update_file = files->head;

    while (update_file != NULL) {
        // reset to head on each iteration
        archive_file = archive_files.head;
        int found = 0;

        // iterate through archived files
        while (archive_file != NULL) {
            if (strcmp(archive_file->name, update_file->name) == 0) {
                // File found in the archive
                found = 1;
                break;
            }
            archive_file = archive_file->next;
        }

        // if the file that needs to be updated is not found, return error
        if (!found) {
            fclose(archive);
            file_list_clear(&archive_files);
            printf("Error: One or more of the specified files is not already present in archive");
            return -1;
        }

        // iterate to next file to update
        update_file = update_file->next;
    }
    fclose(archive);
    file_list_clear(&archive_files);

    return append_files_to_archive(archive_name, files);
}

int main(int argc, char **argv) {
    if (argc < 4) {
        printf("Usage: %s -c|a|t|u|x -f ARCHIVE [FILE...]\n", argv[0]);
        return 0;
    }

    file_list_t files;
    file_list_init(&files);

    char *cmd = argv[1];
    char *archive_name = argv[3];

    for (int i = 4; i < argc; i++) {
        file_list_add(&files, argv[i]);
    }

    if (strcmp(cmd, "-c") == 0) {
        create_archive(archive_name, &files);
    } else if (strcmp(cmd, "-a") == 0) {
        append_files_to_archive(archive_name, &files);
    } else if (strcmp(cmd, "-t") == 0) {
        get_archive_file_list(archive_name, &files);
        node_t *curr = files.head;
        for (int i = 0; i < files.size; i++) {
            printf("%s\n", curr->name);
            curr = curr->next;
        }
    } else if (strcmp(cmd, "-u") == 0) {
        update_archive(archive_name, &files);
    } else if (strcmp(cmd, "-x") == 0) {
        extract_files_from_archive(archive_name);
    } else {
        printf("Unknown command: %s\n", cmd);
        file_list_clear(&files);
        return -1;
    }

    file_list_clear(&files);
    return 0;
}

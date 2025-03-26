#include "minitar.h"

#include <fcntl.h>
#include <grp.h>
#include <math.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#define NUM_TRAILING_BLOCKS 2
#define MAX_MSG_LEN 128
#define BLOCK_SIZE 512

// Constants for tar compatibility information
#define MAGIC "ustar"

// Constants to represent different file types
// We'll only use regular files in this project
#define REGTYPE '0'
#define DIRTYPE '5'

/*
 * Helper function to compute the checksum of a tar header block
 * Performs a simple sum over all bytes in the header in accordance with POSIX
 * standard for tar file structure.
 */
void compute_checksum(tar_header *header) {
    // Have to initially set header's checksum to "all blanks"
    memset(header->chksum, ' ', 8);
    unsigned sum = 0;
    char *bytes = (char *) header;
    for (int i = 0; i < sizeof(tar_header); i++) {
        sum += bytes[i];
    }
    snprintf(header->chksum, 8, "%07o", sum);
}

/*
 * Populates a tar header block pointed to by 'header' with metadata about
 * the file identified by 'file_name'.
 * Returns 0 on success or -1 if an error occurs
 */
int fill_tar_header(tar_header *header, const char *file_name) {
    memset(header, 0, sizeof(tar_header));
    char err_msg[MAX_MSG_LEN];
    struct stat stat_buf;
    // stat is a system call to inspect file metadata
    if (stat(file_name, &stat_buf) != 0) {
        snprintf(err_msg, MAX_MSG_LEN, "Failed to stat file %s", file_name);
        perror(err_msg);
        return -1;
    }

    strncpy(header->name, file_name, 100);    // Name of the file, null-terminated string
    snprintf(header->mode, 8, "%07o",
             stat_buf.st_mode & 07777);    // Permissions for file, 0-padded octal

    snprintf(header->uid, 8, "%07o", stat_buf.st_uid);    // Owner ID of the file, 0-padded octal
    struct passwd *pwd = getpwuid(stat_buf.st_uid);       // Look up name corresponding to owner ID
    if (pwd == NULL) {
        snprintf(err_msg, MAX_MSG_LEN, "Failed to look up owner name of file %s", file_name);
        perror(err_msg);
        return -1;
    }
    strncpy(header->uname, pwd->pw_name, 32);    // Owner name of the file, null-terminated string

    snprintf(header->gid, 8, "%07o", stat_buf.st_gid);    // Group ID of the file, 0-padded octal
    struct group *grp = getgrgid(stat_buf.st_gid);        // Look up name corresponding to group ID
    if (grp == NULL) {
        snprintf(err_msg, MAX_MSG_LEN, "Failed to look up group name of file %s", file_name);
        perror(err_msg);
        return -1;
    }
    strncpy(header->gname, grp->gr_name, 32);    // Group name of the file, null-terminated string

    snprintf(header->size, 12, "%011o",
             (unsigned) stat_buf.st_size);    // File size, 0-padded octal
    snprintf(header->mtime, 12, "%011o",
             (unsigned) stat_buf.st_mtime);    // Modification time, 0-padded octal
    header->typeflag = REGTYPE;                // File type, always regular file in this project
    strncpy(header->magic, MAGIC, 6);          // Special, standardized sequence of bytes
    memcpy(header->version, "00", 2);          // A bit weird, sidesteps null termination
    snprintf(header->devmajor, 8, "%07o",
             major(stat_buf.st_dev));    // Major device number, 0-padded octal
    snprintf(header->devminor, 8, "%07o",
             minor(stat_buf.st_dev));    // Minor device number, 0-padded octal

    compute_checksum(header);
    return 0;
}

/*
 * Removes 'nbytes' bytes from the file identified by 'file_name'
 * Returns 0 upon success, -1 upon error
 * Note: This function uses lower-level I/O syscalls (not stdio), which we'll learn about later
 */
int remove_trailing_bytes(const char *file_name, size_t nbytes) {
    char err_msg[MAX_MSG_LEN];

    struct stat stat_buf;
    if (stat(file_name, &stat_buf) != 0) {
        snprintf(err_msg, MAX_MSG_LEN, "Failed to stat file %s", file_name);
        perror(err_msg);
        return -1;
    }

    off_t file_size = stat_buf.st_size;
    if (nbytes > file_size) {
        file_size = 0;
    } else {
        file_size -= nbytes;
    }

    if (truncate(file_name, file_size) != 0) {
        snprintf(err_msg, MAX_MSG_LEN, "Failed to truncate file %s", file_name);
        perror(err_msg);
        return -1;
    }
    return 0;
}





int write_files_to_archive(const char *archive_name, const file_list_t *files, const int create) {
    char err_msg[MAX_MSG_LEN];

    // either creates/overwrites or appends
    char procedure[3];
    if (create) {
        strncpy(procedure, "wb", 3);
    } else {
        strncpy(procedure, "ab", 3);
    }

    // open archive
    FILE *archive = fopen(archive_name, procedure);
    if (!archive) {
        snprintf(err_msg, MAX_MSG_LEN, "Failed to open archive file: %s", archive_name);
        perror(err_msg);
        return -1;
    }

    node_t *curr = files->head;
    while (curr != NULL) {
        // opens file
        FILE *src = fopen(curr->name, "rb");
        if (!src) {
            snprintf(err_msg, MAX_MSG_LEN, "Failed to open source file: %s", curr->name);
            perror(err_msg);
            fclose(archive);
            return -1;
        }

        // gets file size
        if (fseek(src, 0, SEEK_END) != 0) {
            perror("Failed to seek to end of file");
            fclose(src);
            fclose(archive);
            return -1;
        }
        long file_size = ftell(src);
        if (file_size == -1) {
            perror("Failed to get file size");
            fclose(src);
            fclose(archive);
            return -1;
        }
        fseek(src, 0, SEEK_SET);

        // Create header
        tar_header *header = malloc(sizeof(tar_header));
        if (!header) {
            perror("Failed to allocate memory for header");
            fclose(src);
            fclose(archive);
            return -1;
        }

        if (fill_tar_header(header, curr->name) != 0) {
            free(header);
            fclose(src);
            fclose(archive);
            return -1;
        }

        // Compute checksum
        compute_checksum(header);

        // Write header
        if (fwrite(header, sizeof(tar_header), 1, archive) == 0) {
            perror ("unable to write header to archive file");
            return -1;
        }

        // Write file content
        char buffer[BLOCK_SIZE];
        size_t bytes_read;
        while ((bytes_read = fread(buffer, 1, BLOCK_SIZE, src)) > 0) {
            if (fwrite(buffer, 1, bytes_read, archive) == 0) {
                perror ("unable to write file contents to archive file");
                return -1;
            }
        }

        // File padding
        size_t padding_size = (BLOCK_SIZE - (file_size % BLOCK_SIZE)) % BLOCK_SIZE;
        if (padding_size > 0) {
            char padding[BLOCK_SIZE] = {0};
            if (fwrite(padding, 1, padding_size, archive) == 0) {
                perror ("unable to write file padding to archive file");
                return -1;
            }
        }

        free(header);
        fclose(src);
        curr = curr->next;
    }

    // Write two empty blocks to signify end of archive
    char empty_block[BLOCK_SIZE] = {0};
    fwrite(empty_block, 1, sizeof(empty_block), archive);
    fwrite(empty_block, 1, sizeof(empty_block), archive);

    fclose(archive);

    return 0;
}

int create_archive(const char *archive_name, const file_list_t *files) {
    return write_files_to_archive(archive_name, files, 1);
}

// int update_archive(const char *archive_name, const file_list_t *files) {
//     FILE *archive = fopen(archive_name, "rb");
//     if (!archive) {
//         perror("Failed to open archive file: %s", archive_name);
//         return -1;
//     }

//     file_list_t archive_files;
//     file_list_init(&archive_files);

//     get_archive_file_list(archive_name, &archive_files);

//     // compares files from archive and files to update
//     node_t *archive_file
//     node_t *update_file = files->head;
//     while(update_file != NULL) {
//         if (archive_file) == NULL {
//             return -1;
//         } else {
//             if (archive_file->name != update_file->name) {
//                 return -1;
//             }
//         }
//         update_file = update_file->next;
//         archive_file = archive_file->next;
//     }
//     return 0;
// }

int append_files_to_archive(const char *archive_name, const file_list_t *files) {

    remove_trailing_bytes(archive_name, BLOCK_SIZE * NUM_TRAILING_BLOCKS);
    return write_files_to_archive(archive_name, files, 0);
}


int get_archive_file_list(const char *archive_name, file_list_t *files) {
    FILE *archive = fopen(archive_name, "rb");
    if (!archive) {
        perror("Failed to open archive file");
        return -1;
    }

    tar_header *header = malloc(sizeof(tar_header));
    char block[BLOCK_SIZE] = {0};

    int read_status = 1;
    int file_size;
    int num_blocks;

    while (read_status == 1) {
        read_status = fread(header, sizeof(tar_header), 1, archive);

        // check if the block is all zeros (possible first footer block)
        if ((int)memcmp(header, block, BLOCK_SIZE) == 0) {
            // read the next block to confirm it's also all zeros
            read_status = fread(header, sizeof(tar_header), 1, archive);
            if (read_status != 1) {
                perror("unable to read given archive file, footers may not be correctly formatted");
                free(header);
                fclose(archive);
                return -1;
            }

            if (read_status == 1 && (int)memcmp(header, block, BLOCK_SIZE) == 0) {
                free(header);
                fclose(archive);
                return 0;
            }
            // if it's not a second zero block, print error
            perror("unexpected all zero block found in tar file");
            free(header);
            fclose(archive);
            return -1;
        }

        // add the filename to the list
        file_list_add(files, header->name);

        // convert file size from octal to long int for usability
        file_size = strtol(header->size, NULL, 8);

        // determine number of 512 blocks of content that follow after this header
        num_blocks = (int)ceil((double)file_size / BLOCK_SIZE);

        fseek(archive, num_blocks * BLOCK_SIZE, SEEK_CUR);
    }
    free(header);
    fclose(archive);
    return -1;
}

int extract_files_from_archive(const char *archive_name) {
//     printf("Extracting files from archive: %s\n", archive_name);

//     // Open the archive file in read-binary mode
//     FILE *archive = fopen(archive_name, "rb");
//     if (!archive) {
//         perror("Failed to open archive file");
//         return -1;
//     }

//     tar_header header;
//     char buffer[BLOCK_SIZE];
//     size_t bytes_read;

//     while (1) {
//         // Read the header block
//         bytes_read = fread(&header, sizeof(tar_header), 1, archive);
//         if (bytes_read != 1) {
//             if (feof(archive)) {
//                 break; // End of archive
//             }
//             perror("Failed to read header from archive");
//             fclose(archive);
//             return -1;
//         }

//         // Check if the header block is empty (end of archive)
//         if (header.name[0] == '\0') {
//             break;
//         }

//         // Convert file size from octal string to integer
//         size_t file_size = strtol(header.size, NULL, 8);

//         // Print the file name and size for debugging
//         printf("Extracting file: %s (size: %zu bytes)\n", header.name, file_size);

//         // Open the output file for writing
//         FILE *output_file = fopen(header.name, "wb");
//         if (!output_file) {
//             fprintf(stderr, "Failed to create file %s: %s\n", header.name, strerror(errno));
//             fclose(archive);
//             return -1;
//         }

//         // Read and write the file content
//         size_t remaining_bytes = file_size;
//         while (remaining_bytes > 0) {
//             size_t bytes_to_read = (remaining_bytes < BLOCK_SIZE) ? remaining_bytes : BLOCK_SIZE;
//             bytes_read = fread(buffer, 1, bytes_to_read, archive);
//             if (bytes_read != bytes_to_read) {
//                 perror("Failed to read file content from archive");
//                 fclose(output_file);
//                 fclose(archive);
//                 return -1;
//             }
//             fwrite(buffer, 1, bytes_read, output_file);
//             remaining_bytes -= bytes_read;
//         }

//         // Close the output file
//         fclose(output_file);

//         // Skip padding after the file content
//         size_t padding_size = (BLOCK_SIZE - (file_size % BLOCK_SIZE)) % BLOCK_SIZE;
//         if (padding_size > 0) {
//             fseek(archive, padding_size, SEEK_CUR);
//         }
//     }

//     // Close the archive file
//     fclose(archive);

    printf("Extraction complete.\n");
    return 0;
}

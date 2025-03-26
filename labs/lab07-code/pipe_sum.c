#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

/*
 * Writes a sequence of cumulative sums to a pipe.
 * file_name: The name of the text file from which to read a number sequence
 * pipe_write_fd: File descriptor of pipe to write to
 * Returns 0 on success or -1 on error
 */
int write_sums_to_pipe(const char *file_name, int pipe_write_fd) {
    FILE *f = fopen(file_name, "r");
    if (f == NULL) {
        perror("fopen");
        return -1;
    }

    int sum = 0;
    int n;
    while (fscanf(f, "%d", &n) > 0) {
        sum += n;
        // TODO write cumulative sum to pipe
        if (write(pipe_write_fd, &sum, sizeof(sum)) == -1) {
            perror("write");
            fclose(f);
            return -1;
        }
    }

    if (ferror(f)) {
        perror("Failed to read file");
        fclose(f);
        return -1;
    }
    if (fclose(f) == EOF) {
        perror("Failed to close input file");
        return -1;
    }
    return 0;
}

/*
 * Read a sequence of cumulative sums from a pipe
 * pipe_read_fd: File descriptor of the pipe to read from
 * Returns 0 on success or -1 on error
 */
int read_sums_from_pipe(int pipe_read_fd) {
    // TODO read all sums from pipe
    int sum;
    while (read(pipe_read_fd, &sum, sizeof(sum)) > 0) {
        printf("Cumulative Sum: %d\n", sum);
    }
    close(pipe_read_fd);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s <numbers_file>\n", argv[0]);
        return 1;
    }
    // Uncomment line below if you would like to use it
    const char *file_name = argv[1];

    // TODO set up pipe file descriptors
    int pipe_fds[2];
    if (pipe(pipe_fds) == -1) {
        perror("pipe");
        return 1;
    }

    pid_t child_pid = fork();
    if (child_pid == -1) {
        perror("fork");
        return 1;
    }

    else if (child_pid == 0) {
        // TODO write code to do the following:
        // 1. Close unused pipe file descriptors
        // 2. Call 'write_sums_to_pipe' with appropriate arguments
        // 3. Close remaining pipe file descriptors
        close(pipe_fds[0]);
        write_sums_to_pipe(file_name, pipe_fds[1]);
        close(pipe_fds[1]);
    }

    else {
        // TODO write code to do the following:
        // 1. Close unused pipe file descriptors
        // 2. Call 'read_sums_from_pipe' with appropriate arguments
        // 3. Close remaining pipe file descriptors
        close(pipe_fds[1]);
        read_sums_from_pipe(pipe_fds[0]);
        close(pipe_fds[0]);
    }

    return 0;
}

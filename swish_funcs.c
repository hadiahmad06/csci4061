#define _GNU_SOURCE

#include "swish_funcs.h"

#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "job_list.h"
#include "string_vector.h"

#define MAX_ARGS 10

int tokenize(char *s, strvec_t *tokens) {
    // TODO Task 0: Tokenize string s

    const char *delim = " ";
    char *curr = strtok(s, delim);

    while(curr != NULL) {
        strvec_add(tokens, curr);
        curr = strtok(NULL, delim);
    }

    // Assume each token is separated by a single space (" ")
    // Use the strtok() function to accomplish this
    // Add each token to the 'tokens' parameter (a string vector)
    // Return 0 on success, -1 on error
    return 0;
}

int run_command(strvec_t *tokens) {
    // TODO Task 2: Execute the specified program (token 0) with the
    // specified command-line arguments
    // THIS FUNCTION SHOULD BE CALLED FROM A CHILD OF THE MAIN SHELL PROCESS
    // Hint: Build a string array from the 'tokens' vector and pass this into execvp()
    // Another Hint: You have a guarantee of the longest possible needed array, so you
    // won't have to use malloc.

    int argc = tokens->length;
    char *argv[argc+1];

    // redirection file descriptors
    int input_fd = -1;
    int output_fd = -1;
    int skip_indices[4] = {-1,-1,-1,-1};
    int skip_count = 0;

    // locates a redirection file operator and sets index
    for (int i = 0; i < argc; i++) {
        if (i + 1 < argc) {
            if (strcmp(tokens->data[i], ">") == 0) {
                output_fd = open(tokens->data[i + 1], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
                if (output_fd == -1) {
                    perror("Failed to open output file");
                    return -1;
                }
            } else if (strcmp(tokens->data[i], ">>") == 0) {
                output_fd = open(tokens->data[i + 1], O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
                if (output_fd == -1) {
                    perror("Failed to open output file");
                    return -1;
                }
            } else if (strcmp(tokens->data[i], "<") == 0) {
                input_fd = open(tokens->data[i + 1], O_RDONLY);
                if (input_fd == -1) {
                    perror("Failed to open input file");
                    return -1;
                }
            }
            if (output_fd != -1 || input_fd != -1) {
                skip_indices[skip_count++] = i;
                skip_indices[skip_count++] = i + 1;
            }
        }
    }

    // perform file redirection if necessary
    if (input_fd != -1) {
        dup2(input_fd, STDIN_FILENO);
        close(input_fd);
    }
    if (output_fd != -1) {
        dup2(output_fd, STDOUT_FILENO);
        close(output_fd);
    }

    // skip redirection tokens
    int j = 0;
    for (int i = 0; i < argc; i++) {
        int skip = 0;
        for (int k = 0; k < skip_count; k++) {
            if (i == skip_indices[k]) {
                skip = 1;
                break;
            }
        }
        if (!skip) {
            argv[j] = tokens->data[i];
            j++;
        }
    }
    argv[j] = NULL;

    struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGTTOU, &sa, NULL);
    sigaction(SIGTTIN, &sa, NULL);

    // change process group
    pid_t pid = getpid();
    setpgid(pid, pid);

    // executes command
    execvp(argv[0], argv);

    // error check
    perror("exec");
    _exit(1);

    // TODO Task 3: Extend this function to perform output redirection before exec()'ing
    // Check for '<' (redirect input), '>' (redirect output), '>>' (redirect and append output)
    // entries inside of 'tokens' (the strvec_find() function will do this for you)
    // Open the necessary file for reading (<), writing (>), or appending (>>)
    // Use dup2() to redirect stdin (<), stdout (> or >>)
    // DO NOT pass redirection operators and file names to exec()'d program
    // E.g., "ls -l > out.txt" should be exec()'d with strings "ls", "-l", NULL

    // TODO Task 4: You need to do two items of setup before exec()'ing
    // 1. Restore the signal handlers for SIGTTOU and SIGTTIN to their defaults.
    // The code in main() within swish.c sets these handlers to the SIG_IGN value.
    // Adapt this code to use sigaction() to set the handlers to the SIG_DFL value.
    // 2. Change the process group of this process (a child of the main shell).
    // Call getpid() to get its process ID then call setpgid() and use this process
    // ID as the value for the new process group ID

    return 0;
}

int resume_job(strvec_t *tokens, job_list_t *jobs, int is_foreground) {
    if (tokens->length < 2) {
        fprintf(stderr, "Usage: fg <job_index> or bg <job_index>\n");
        return -1;
    }

    int job_index = atoi(strvec_get(tokens, 1));
    job_t *job = job_list_get(jobs, job_index);
    if (job == NULL) {
        fprintf(stderr, "Job index out of bounds\n");
        return -1;
    }

    pid_t job_pid = job->pid;
    pid_t shell_pid = getpid();

    if (is_foreground) {
        // Set the job's process group as the foreground process group
        if (tcsetpgrp(STDIN_FILENO, job_pid) == -1) {
            perror("tcsetpgrp");
            return -1;
        }

        // Send SIGCONT to the job's process group
        if (kill(-job_pid, SIGCONT) == -1) {
            perror("kill");
            return -1;
        }

        // Wait for the job to finish or stop
        int status;
        if (waitpid(job_pid, &status, WUNTRACED) == -1) {
            perror("waitpid");
            return -1;
        }

        // Restore the shell's process group as the foreground process group
        if (tcsetpgrp(STDIN_FILENO, shell_pid) == -1) {
            perror("tcsetpgrp");
            return -1;
        }

        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            // Job has terminated, remove it from the jobs list
            job_list_remove(jobs, job_index);
        } else if (WIFSTOPPED(status)) {
            // Job has stopped again, update its status
            job->status = STOPPED;
        }
    } else {
        // Resume the job in the background
        if (kill(-job_pid, SIGCONT) == -1) {
            perror("kill");
            return -1;
        }

        // Update the job's status to BACKGROUND
        job->status = BACKGROUND;
    }

    return 0;
}

int await_background_job(strvec_t *tokens, job_list_t *jobs) {
    if (tokens->length < 2) {
        fprintf(stderr, "Usage: wait-for <job_index>\n");
        return -1;
    }

    int job_index = atoi(strvec_get(tokens, 1));
    job_t *job = job_list_get(jobs, job_index);
    if (job == NULL) {
        fprintf(stderr, "Job index out of bounds\n");
        return -1;
    }

    // Check if the job is a background job
    if (job->status != BACKGROUND) {
        fprintf(stderr, "Job index is for stopped process not background process\n"); // Correct error message
        return -1;
    }

    // Wait for the background job to finish or stop
    int status;
    if (waitpid(job->pid, &status, WUNTRACED) == -1) {
        perror("waitpid");
        return -1;
    }

    // If the job has terminated, remove it from the jobs list
    if (WIFEXITED(status) || WIFSIGNALED(status)) {
        job_list_remove(jobs, job_index);
    }

    return 0;
}

int await_all_background_jobs(job_list_t *jobs) {
    if (jobs == NULL || jobs->head == NULL) {
        // No jobs to wait for
        return 0;
    }

    // Step 1: Iterate through the jobs list, ignoring stopped jobs
    job_t *current = jobs->head;
    while (current != NULL) {
        if (current->status == BACKGROUND) {
            // Step 2: Call waitpid() with WUNTRACED for background jobs
            int status;
            if (waitpid(current->pid, &status, WUNTRACED) == -1) {
                perror("waitpid");
                return -1;
            }

            // Step 3: Handle stopped jobs
            if (WIFSTOPPED(status)) {
                // If the job has stopped, change its status to STOPPED
                current->status = STOPPED;
            }
        }

        // Move to the next job
        current = current->next;
    }

    // Step 4: Remove all terminated background jobs from the jobs list
    job_list_remove_by_status(jobs, BACKGROUND);

    return 0;
}

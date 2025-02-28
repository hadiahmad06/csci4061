#define _GNU_SOURCE

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "job_list.h"
#include "string_vector.h"
#include "swish_funcs.h"

#define CMD_LEN 512
#define PROMPT "@> "

int main(int argc, char **argv) {
    // Task 4: Set up shell to ignore SIGTTIN, SIGTTOU when put in background
    struct sigaction sac;
    sac.sa_handler = SIG_IGN;
    if (sigfillset(&sac.sa_mask) == -1) {
        perror("sigfillset");
        return 1;
    }
    sac.sa_flags = 0;
    if (sigaction(SIGTTIN, &sac, NULL) == -1 || sigaction(SIGTTOU, &sac, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    strvec_t tokens;
    strvec_init(&tokens);
    job_list_t jobs;
    job_list_init(&jobs);
    char cmd[CMD_LEN];

    printf("%s", PROMPT);
    while (fgets(cmd, CMD_LEN, stdin) != NULL) {
        // Remove trailing '\n' from cmd
        int i = 0;
        while (cmd[i] != '\n') {
            i++;
        }
        cmd[i] = '\0';

        if (tokenize(cmd, &tokens) != 0) {
            printf("Failed to parse command\n");
            strvec_clear(&tokens);
            job_list_free(&jobs);
            return 1;
        }
        if (tokens.length == 0) {
            printf("%s", PROMPT);
            continue;
        }
        const char *first_token = strvec_get(&tokens, 0);

        if (strcmp(first_token, "pwd") == 0) {
            char cwd[CMD_LEN];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                printf("%s\n", cwd);
            } else {
                perror("cwd");
            }
        }

        else if (strcmp(first_token, "cd") == 0) {
            // if parameter given, dir = parameter, else, use NULL
            const char *dir = (tokens.length > 1) ? strvec_get(&tokens, 1) : getenv("HOME");
            if (dir == NULL) {
                fprintf(stderr, "cd: HOME not set\n");
            } else if (chdir(dir) != 0) {
                perror("chdir");
            }
        }

        else if (strcmp(first_token, "exit") == 0) {
            strvec_clear(&tokens);
            break;
            // ends loop and clears tokens
        }

        // Task 5: Print out current list of pending jobs
        else if (strcmp(first_token, "jobs") == 0) {
            int i = 0;
            job_t *current = jobs.head;
            while (current != NULL) {
                char *status_desc;
                if (current->status == BACKGROUND) {
                    status_desc = "background";
                } else {
                    status_desc = "stopped";
                }
                printf("%d: %s (%s)\n", i, current->name, status_desc);
                i++;
                current = current->next;
            }
        }

        // Task 5: Move stopped job into foreground
        else if (strcmp(first_token, "fg") == 0) {
            if (resume_job(&tokens, &jobs, 1) == -1) {
                printf("Failed to resume job in foreground\n");
            }
        }

        // Task 6: Move stopped job into background
        else if (strcmp(first_token, "bg") == 0) {
            if (resume_job(&tokens, &jobs, 0) == -1) {
                printf("Failed to resume job in background\n");
            }
        }

        // Task 6: Wait for a specific job identified by its index in job list
        else if (strcmp(first_token, "wait-for") == 0) {
            if (await_background_job(&tokens, &jobs) == -1) {
                printf("Failed to wait for background job\n");
            }
        }

        // Task 6: Wait for all background jobs
        else if (strcmp(first_token, "wait-all") == 0) {
            if (await_all_background_jobs(&jobs) == -1) {
                printf("Failed to wait for all background jobs\n");
            }
        }

        else {
            // checks for & at end of cmd (background job)
            int is_background = 0;
            if (tokens.length > 0 && strcmp(strvec_get(&tokens, tokens.length - 1), "&") == 0) {
                is_background = 1;
                strvec_take(&tokens, tokens.length - 1); // removes & from tokens
            }

            pid_t pid = fork();
            if (pid == -1) {
                perror("fork");
                strvec_clear(&tokens);
                job_list_free(&jobs);
                return 1;
            }

            if (pid == 0) {
                // exec in child process
                run_command(&tokens);
                exit(EXIT_FAILURE);
                // if exec fails, exit the child process
            } else {
                // parent handles jobs



                if (!is_background) {
                    // sets child process to foreground process group
                    if (tcsetpgrp(STDIN_FILENO, pid) == -1) {
                        perror("tcsetpgrp");
                    }

                    // waits for child process to stop
                    int status;
                    if (waitpid(pid, &status, WUNTRACED) == -1) {
                        perror("waitpid");
                    }

                    // when finished, sets parent (shell) process to foreground process group
                    if (tcsetpgrp(STDIN_FILENO, getpid()) == -1) {
                        perror("tcsetpgrp");
                    }

                    // if child was stopped, add to job list with stopped status
                    if (WIFSTOPPED(status)) {
                        job_list_add(&jobs, pid, strvec_get(&tokens, 0), STOPPED);
                    }
                } else {
                    // add child process to job list with background status
                    job_list_add(&jobs, pid, strvec_get(&tokens, 0), BACKGROUND);
                }
            }
        }

        printf("%s", PROMPT);
        strvec_clear(&tokens);
    }

    job_list_free(&jobs);
    return 0;
}

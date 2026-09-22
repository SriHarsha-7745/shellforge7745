#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include "executor.h"
#include "builtin.h"

/* ---- SIGCHLD handling: reap finished background jobs so they never
 * become zombies (Milestone 4.2, slide 10) ---- */
static void sigchld_handler(int sig) {
    int saved_errno = errno;
    (void)sig;
    while (waitpid(-1, NULL, WNOHANG) > 0) {
        /* reap any finished background child */
    }
    errno = saved_errno;
}

void setup_background_handler(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);
}

/* waitpid() wrapper that tolerates a child the SIGCHLD handler above
 * already reaped out from under us -- without this, a fast-finishing
 * foreground child can race the handler and make waitpid() fail with
 * ECHILD even though the command ran fine. */
static int wait_for_pid(pid_t pid) {
    int status;
    pid_t r;
    do {
        r = waitpid(pid, &status, 0);
    } while (r == -1 && errno == EINTR);

    if (r == -1) {
        /* already reaped by the SIGCHLD handler; treat as success
         * since we can no longer retrieve its real exit status */
        return 0;
    }
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

/* ---- apply_redirection(): opens cmd->input/output (if set) and
 * dup2()s them onto STDIN/STDOUT. Must be called from inside a child,
 * AFTER any pipe dup2() so explicit redirection wins over a pipe. ---- */
static void apply_redirection(command_t *cmd) {
    if (cmd->input[0] != '\0') {
        int fd = open(cmd->input, O_RDONLY);
        if (fd < 0) {
            perror(cmd->input);
            _exit(1);
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }

    if (cmd->output[0] != '\0') {
        int flags = O_WRONLY | O_CREAT | (cmd->append ? O_APPEND : O_TRUNC);
        int fd = open(cmd->output, flags, 0644);
        if (fd < 0) {
            perror(cmd->output);
            _exit(1);
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
}

/* Background children shouldn't read from the terminal's keyboard
 * input -- redirect their stdin to /dev/null (slide 8/12). */
static void redirect_stdin_devnull(void) {
    int null_fd = open("/dev/null", O_RDONLY);
    if (null_fd != -1) {
        dup2(null_fd, STDIN_FILENO);
        close(null_fd);
    }
}

/* ---- execute_command(): runs ONE command, no pipes involved ---- */
int execute_command(command_t *cmd) {
    if (cmd == NULL || cmd->argc == 0) {
        return -1;
    }

    if (cmd->background) {
        /* Backgrounded commands are always forked -- even builtins --
         * since there is no shell process left waiting to inherit
         * a builtin's in-process effect (e.g. "cd &" can't usefully
         * change the parent's directory asynchronously). */
        fflush(stdout);
        pid_t pid = fork();

        if (pid == 0) {
            redirect_stdin_devnull();
            apply_redirection(cmd);

            if (is_builtin(cmd->argv[0])) {
                execute_builtin(cmd);
                fflush(stdout);
                _exit(0);
            }

            execvp(cmd->argv[0], cmd->argv);
            perror("shellforge");
            _exit(127);
        } else if (pid > 0) {
            printf("[Background PID: %d]\n", pid);
            return 0; /* do NOT wait */
        }

        perror("fork");
        return -1;
    }

    if (is_builtin(cmd->argv[0])) {
        /* foreground builtins run in the shell's own process (not
         * forked), so "cd" actually changes the shell's directory */
        return execute_builtin(cmd);
    }

    pid_t pid = fork();

    if (pid == 0) {
        apply_redirection(cmd);
        execvp(cmd->argv[0], cmd->argv);
        perror("shellforge");
        _exit(127);
    } else if (pid > 0) {
        return wait_for_pid(pid);
    }

    perror("fork");
    return -1;
}

/* ---- execute_pipeline(): runs a chain of commands, wiring the
 * stdout of each into the stdin of the next with pipe()+dup2() ---- */
int execute_pipeline(pipeline_t *pipeline) {
    if (pipeline == NULL || pipeline->command_count == 0) {
        return -1;
    }

    int n = pipeline->command_count;

    if (n == 1) {
        return execute_command(&pipeline->commands[0]);
    }

    int background = pipeline->commands[n - 1].background;

    /* --- multi-command pipeline: cmd0 | cmd1 | ... | cmd(n-1) ---
     * "previous_read" carries the read end of the pipe from the
     * previous stage forward, one pipe at a time. */
    int previous_read = -1;
    pid_t pids[MAX_COMMANDS];
    int last_status = -1;

    for (int i = 0; i < n; i++) {
        command_t *cmd = &pipeline->commands[i];
        int is_last = (i == n - 1);
        int pipefd[2] = { -1, -1 };

        if (!is_last && pipe(pipefd) < 0) {
            perror("pipe");
            pids[i] = -1;
            continue;
        }

        if (cmd->argc == 0) {
            pids[i] = -1;
        } else {
            fflush(stdout); /* avoid duplicating buffered output into the child */
            pid_t pid = fork();

            if (pid == 0) {
                /* Child i:
                 *   i > 0     -> STDIN  comes from previous pipe's read end
                 *   i < n-1   -> STDOUT goes to this pipe's write end
                 */
                if (previous_read != -1) {
                    dup2(previous_read, STDIN_FILENO);
                }
                if (!is_last) {
                    dup2(pipefd[1], STDOUT_FILENO);
                }

                /* close every fd this child no longer needs */
                if (previous_read != -1) {
                    close(previous_read);
                }
                if (!is_last) {
                    close(pipefd[0]);
                    close(pipefd[1]);
                }

                /* the very first stage of a backgrounded pipeline
                 * shouldn't block reading the terminal's keyboard */
                if (background && i == 0) {
                    redirect_stdin_devnull();
                }

                /* explicit < / > / >> on this segment overrides the
                 * pipe wiring above (matches normal shell behavior) */
                apply_redirection(cmd);

                if (is_builtin(cmd->argv[0])) {
                    execute_builtin(cmd);
                    fflush(stdout);
                    _exit(0);
                }

                execvp(cmd->argv[0], cmd->argv);
                perror("shellforge");
                _exit(127);
            } else if (pid > 0) {
                pids[i] = pid;
            } else {
                perror("fork");
                pids[i] = -1;
            }
        }

        /* parent: close what it no longer needs, keep the read end
         * alive to hand to the next command in the loop */
        if (previous_read != -1) {
            close(previous_read);
        }
        if (!is_last) {
            close(pipefd[1]);
            previous_read = pipefd[0];
        } else {
            previous_read = -1;
        }
    }

    if (background) {
        /* don't wait on any child; report the PID of the FIRST
         * process in the pipeline (slide 9) */
        printf("[Background Pipeline PID: %d]\n", pids[0]);
        return 0;
    }

    for (int i = 0; i < n; i++) {
        if (pids[i] <= 0) {
            continue;
        }
        int status = wait_for_pid(pids[i]);
        if (i == n - 1) {
            last_status = status;
        }
    }

    return last_status;
}

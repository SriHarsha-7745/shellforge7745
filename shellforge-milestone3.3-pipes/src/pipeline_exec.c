#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "pipeline_exec.h"
#include "builtin.h"

/* Runs a single command that sits inside a multi-command pipeline.
 * Always called from inside a forked child. Uses _exit(), not exit(),
 * so the child never re-flushes stdio buffers it copied from the
 * parent at fork() time (that bug shows up as duplicated shell output). */
static void run_child(command_t *cmd, int in_fd, int out_fd,
                       int *pipefds, int npipefds) {
    if (in_fd != STDIN_FILENO) {
        dup2(in_fd, STDIN_FILENO);
    }
    if (out_fd != STDOUT_FILENO) {
        dup2(out_fd, STDOUT_FILENO);
    }

    /* every pipe fd must be closed in every child, not just the ends
     * it was handed, or downstream readers will never see EOF */
    for (int i = 0; i < npipefds; i++) {
        close(pipefds[i]);
    }

    if (is_builtin(cmd->argv[0])) {
        execute_builtin(cmd);
        fflush(stdout);
        _exit(0); /* "exit" inside a pipe segment just ends that segment */
    }

    execvp(cmd->argv[0], cmd->argv);
    perror("shellforge");
    _exit(1);
}

int execute_pipeline(pipeline_t *pipeline) {
    int n = pipeline->command_count;

    /* Fast path: a single, non-piped command. Builtins run directly in
     * the shell process itself (no fork) so that things like "cd"
     * actually change the shell's own working directory. */
    if (n == 1) {
        command_t *cmd = &pipeline->commands[0];
        if (cmd->argc == 0) {
            return 0;
        }
        if (is_builtin(cmd->argv[0])) {
            return execute_builtin(cmd) == 1 ? 1 : 0;
        }

        pid_t pid = fork();
        if (pid == 0) {
            execvp(cmd->argv[0], cmd->argv);
            perror("shellforge");
            exit(1);
        } else if (pid > 0) {
            if (!cmd->background) {
                int status;
                waitpid(pid, &status, 0);
            } else {
                printf("[background pid %d]\n", pid);
            }
        } else {
            perror("fork");
        }
        return 0;
    }

    /* Multi-command pipeline: cmd0 | cmd1 | ... | cmd(n-1) */
    int npipefds = 2 * (n - 1);
    int pipefds[2 * (MAX_COMMANDS - 1)];

    for (int i = 0; i < n - 1; i++) {
        if (pipe(pipefds + i * 2) < 0) {
            perror("pipe");
            return 0;
        }
    }

    pid_t pids[MAX_COMMANDS];

    fflush(stdout); /* flush once before spawning any child in the pipeline */

    for (int i = 0; i < n; i++) {
        command_t *cmd = &pipeline->commands[i];

        if (cmd->argc == 0) {
            pids[i] = -1;
            continue;
        }

        int in_fd  = (i == 0)     ? STDIN_FILENO  : pipefds[(i - 1) * 2];
        int out_fd = (i == n - 1) ? STDOUT_FILENO : pipefds[i * 2 + 1];

        pid_t pid = fork();
        if (pid == 0) {
            run_child(cmd, in_fd, out_fd, pipefds, npipefds);
            /* run_child never returns */
        } else if (pid > 0) {
            pids[i] = pid;
        } else {
            perror("fork");
            pids[i] = -1;
        }
    }

    /* the parent must close every pipe fd itself, or readers will
     * block forever waiting for an EOF that never comes */
    for (int i = 0; i < npipefds; i++) {
        close(pipefds[i]);
    }

    int background = pipeline->commands[n - 1].background;

    if (!background) {
        for (int i = 0; i < n; i++) {
            if (pids[i] > 0) {
                int status;
                waitpid(pids[i], &status, 0);
            }
        }
    } else {
        printf("[background pipeline, last pid %d]\n", pids[n - 1]);
    }

    return 0;
}

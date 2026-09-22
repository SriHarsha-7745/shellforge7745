#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "builtin.h"

int is_builtin(const char *cmd) {
    if (cmd == NULL) {
        return 0;
    }
    return (strcmp(cmd, "cd") == 0 ||
            strcmp(cmd, "pwd") == 0 ||
            strcmp(cmd, "echo") == 0 ||
            strcmp(cmd, "exit") == 0);
}

static int builtin_cd(command_t *cmd) {
    char *dir;

    if (cmd->argc == 1) {
        dir = getenv("HOME");
        if (dir == NULL) {
            fprintf(stderr, "cd: HOME not set\n");
            return 0;
        }
    } else if (cmd->argc == 2) {
        dir = cmd->argv[1];
    } else {
        fprintf(stderr, "cd: too many arguments\n");
        return 0;
    }

    if (chdir(dir) != 0) {
        perror("cd");
    }
    return 0;
}

static int builtin_pwd(command_t *cmd) {
    if (cmd->argc > 1) {
        fprintf(stderr, "pwd: too many arguments\n");
        return 0;
    }

    char buffer[1024];
    if (getcwd(buffer, sizeof(buffer)) == NULL) {
        perror("pwd");
        return 0;
    }

    printf("This is my pwd: %s\n", buffer);
    return 0;
}

static int builtin_echo(command_t *cmd) {
    for (int i = 1; i < cmd->argc; i++) {
        printf("%s", cmd->argv[i]);
        if (i != cmd->argc - 1) {
            printf(" ");
        }
    }
    printf("\n");
    return 0;
}

static int builtin_exit(command_t *cmd) {
    if (cmd->argc > 1) {
        fprintf(stderr, "exit: too many arguments\n");
    }
    return 1;
}

int execute_builtin(command_t *cmd) {
    if (strcmp(cmd->argv[0], "cd") == 0) {
        return builtin_cd(cmd);
    } else if (strcmp(cmd->argv[0], "pwd") == 0) {
        return builtin_pwd(cmd);
    } else if (strcmp(cmd->argv[0], "echo") == 0) {
        return builtin_echo(cmd);
    } else if (strcmp(cmd->argv[0], "exit") == 0) {
        return builtin_exit(cmd);
    }
    return 0;
}

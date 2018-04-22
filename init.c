#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
/*
issue: 
1. redirect.
*/
pid_t cpid = -1;
char cmd[256];
char *argv[128];

void cmd_parse(char *cmd, char *argv[], char *symbol) {
    if (symbol == NULL) symbol = cmd + 1024;
    memset (argv, 0, 128);
    int i = 0;
    int k = 1;

    int j = 0;
    for (j = 0; cmd[j] == ' '; j++) {
        cmd[j] = '\0';
    }
    argv[0] = &cmd[j];

    for (i = j; cmd[i] != '\0' && cmd + i <= symbol; i++) {
        if (cmd[i] == ' ') {
            while (cmd[i] == ' ') { cmd[i] = '\0'; i++; }
            if (cmd[i] != '\0') argv[k++] = &cmd[i];
        }
    }
}

void exportCmd(char *args) {
    char* p = strchr(args, '=');
    *p = 0;
    setenv(args, p + 1, 1);
}


int cmd_exec(char *argv[]) {
    if (strcmp(argv[0], "cd") == 0) {
        if (argv[1])    chdir(argv[1]);
        return 0;
    }
    else if(strcmp(argv[0], "pwd") == 0) {
        char wc[256];
        puts(getcwd(wc, 256));
        return 0;
    }
    else if(strcmp(argv[0], "exit") == 0) {
        return 128;
    }
    else if (strcmp(argv[0], "export") == 0) {
        exportCmd(argv[1]);
        return 1;
    }
    else {
        cpid = fork();
        if (cpid == 0) {
            execvp(argv[0], &argv[0]);
            return 255;
        } else { 
            waitpid(cpid, NULL, 0);
            return 0;
        }
    }
}


int main () {
    int in = dup(0);
    int out = dup(1);
    while (cpid != 0) {
        fflush(stdout);
        fflush(stdin);
        dup2(in, 0);
        dup2(out, 1);

        printf("%d >> ", getpid());
        memset(cmd, 0, 256);
        memset(argv, 0, 128);

        fgets(cmd, 256, stdin);

        int i = 0;
        for (i = 0; i <= 256; i++)
            if (cmd[i] == '\n') cmd[i] = '\0';

        if (cmd[0] == '0') continue;

        char *symbol = strchr(cmd, '|');
        if (symbol) {
            int pipefd[32][2] = {-1};

            int num = 0;

            pipe(pipefd[num]);

            dup2(pipefd[num][1], 1);
            close(pipefd[num][1]);
            num++;

            char *each_cmd = cmd;
            symbol = strchr(each_cmd, '|');
            *symbol = 0;

            cmd_parse(cmd, argv, symbol);
            cmd_exec(argv);
            if (cpid == 0) return 0;

            each_cmd = symbol + 1; // next

            while(strchr(each_cmd, '|')) {
                pipe(pipefd[num]);

                dup2(pipefd[num - 1][0], 0);
                close(pipefd[num - 1][0]);
                
                symbol = strchr(each_cmd, '|');
                *symbol = 0;
                
                dup2(pipefd[num][1], 1);
                close(pipefd[num][1]);

                num ++;

                cmd_parse(each_cmd, argv, symbol);
                cmd_exec(argv);
                if (cpid == 0) return 0;

                each_cmd = symbol + 1;
            }

            // last
            dup2(out, 1);
            dup2(pipefd[num - 1][0], 0);
            cmd_parse(each_cmd, argv, NULL);
            cmd_exec(argv);
            if (cpid == 0) return 0;

            dup2(in, 0);

            close(pipefd[num - 1][0]);
            close(pipefd[num][1]);
            close(pipefd[num][0]);
        } else {
            cmd_parse(cmd, argv, NULL);
            if(cmd_exec(argv) == 128)
                return 0;
        }

        if (cpid == 0) break;
    }
    return 0;
}
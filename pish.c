#include <ctype.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "pish.h"

/*
 * Batch mode flag. If set to 0, the shell reads from stdin. If set to 1,
 * the shell reads from a file from argv[1].
 */
static int script_mode = 0;

/*
 * Prints a prompt IF NOT in batch mode (see script_mode global flag),
 */ 
int prompt(void)
{
    static const char prompt[] = {0xe2, 0x96, 0xb6, ' ', ' ', '\0'};
    if (!script_mode) {
        printf("%s", prompt);
        fflush(stdout);
    }
    return 1;
}

/*
 * Print usage error for built-in commands.
 */
void usage_error(void)
{
    fprintf(stderr, "pish: Usage error\n");
    fflush(stdout);
}

/*
 * Break down a line of input by whitespace, and put the results into
 * a struct pish_arg to be used by other functions.
 *
 * @param command   A char buffer containing the input command
 * @param arg       Broken down args will be stored here.
 */
void parse_command(char *command, struct pish_arg *arg)
{
    arg->argc = 0;
    for (int i = 0; i < MAX_ARGC; i++) {
        arg->argv[i] = NULL;
    }

    char *tok = strtok(command, " \t");
    while (tok && arg->argc < MAX_ARGC - 1) {
        arg->argv[arg->argc++] = tok;
        tok = strtok(NULL, " \t");
    }
    arg->argv[arg->argc] = NULL;
}

/*
 * Run a command.
 *
 * Built-in commands are handled internally by the pish program.
 * Otherwise, use fork/exec to create child process to run the program.
 *
 * If the command is empty, do nothing.
 * If NOT in batch mode, add the command to history file.
 */
void run(struct pish_arg *arg)
{
    if (arg->argc == 0) {
        return;
    }

    if (!script_mode) {
        add_history(arg);
    }

    if (strcmp(arg->argv[0], "exit") == 0) {
        if (arg->argc != 1) { usage_error(); return; }
        exit(EXIT_SUCCESS);
    }

    if (strcmp(arg->argv[0], "cd") == 0) {
        if (arg->argc != 2) { usage_error(); return; }
        if (chdir(arg->argv[1]) != 0) {
            perror("cd");
        }
        return;
    }

    if (strcmp(arg->argv[0], "history") == 0) {
        if (arg->argc != 1) { usage_error(); return; }
        print_history();
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    }
    if (pid == 0) {
        execvp(arg->argv[0], arg->argv);
        perror("pish");
        _exit(1);
    }
    (void)waitpid(pid, NULL, 0);
}

/*
 * The main loop. Continuously reads input from a FILE pointer
 * (can be stdin or an actual file) until `exit` or EOF.
 */
int pish(FILE *fp)
{
    char buf[1024];
    struct pish_arg arg;

    while (prompt() && fgets(buf, sizeof(buf), fp)) {
        size_t n = strlen(buf);
        if (n > 0 && buf[n - 1] == '\n') buf[n - 1] = '\0';

        int only_ws = 1;
        for (size_t i = 0; buf[i] != '\0'; i++) {
            if (buf[i] != ' ' && buf[i] != '\t') { only_ws = 0; break; }
        }
        if (only_ws) continue;

        char work[1024];
        strncpy(work, buf, sizeof(work) - 1);
        work[sizeof(work) - 1] = '\0';
        parse_command(work, &arg);
        run(&arg);
    }

    exit(EXIT_SUCCESS);
    return 0;
}

int main(int argc, char *argv[])
{
    FILE *fp;

    if (argc == 1) {
        script_mode = 0;
        fp = stdin;
    } else if (argc == 2) {
        script_mode = 1;
        fp = fopen(argv[1], "r");
        if (!fp) { perror("open"); return EXIT_FAILURE; }
    } else {
        usage_error();
        return EXIT_FAILURE;
    }

    pish(fp);

    if (fp && fp != stdin) fclose(fp);

    return EXIT_SUCCESS;
}
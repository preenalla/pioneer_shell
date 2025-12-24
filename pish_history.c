#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "pish.h"

static char pish_history_path[1024] = {'\0'};

/*
 * Set history file path to ~/.pish_history.
 */
static void set_history_path()
{
    const char *home = getpwuid(getuid())->pw_dir;
    strncpy(pish_history_path, home, sizeof(pish_history_path) - 1);
    pish_history_path[sizeof(pish_history_path) - 1] = '\0';
    strncat(pish_history_path, "/.pish_history",
            sizeof(pish_history_path) - strlen(pish_history_path) - 1);
}

void add_history(const struct pish_arg *arg)
{
    if (!(*pish_history_path)) {
        set_history_path();
    }

    if (arg->argc <= 0) return;

    char line[1024] = {0};
    size_t off = 0;
    for (int i = 0; i < arg->argc; i++) {
        const char *w = arg->argv[i] ? arg->argv[i] : "";
        size_t wl = strlen(w);
        if (i > 0 && off + 1 < sizeof(line)) line[off++] = ' ';
        if (off + wl >= sizeof(line)) {
            wl = sizeof(line) - 1 - off;
        }
        memcpy(line + off, w, wl);
        off += wl;
        line[off] = '\0';
        if (off >= sizeof(line) - 1) break;
    }
    if (off == 0) return;

    FILE *f = fopen(pish_history_path, "a");
    if (!f) { perror("open"); return; }
    fprintf(f, "%s\n", line);
    fclose(f);
}

void print_history()
{
    if (!(*pish_history_path)) {
        set_history_path();
    }

    FILE *f = fopen(pish_history_path, "r");
    if (!f) { perror("open"); return; }

    char buf[1024];
    int n = 1;
    while (fgets(buf, sizeof(buf), f)) {
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
        printf("%d %s\n", n++, buf);
    }
    fclose(f);
}
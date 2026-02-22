#include "walk.h"
#include "tokenize.h"

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static void on_word(const char *word, void *ctx) {
    wordmap_add((wordmap_t *)ctx, word, 1);
}

static int scan_file(wordmap_t *m, const char *path) {
    FILE *fp = fopen(path, "rb");
    if (!fp) return 1;
    tokenize_stream(fp, on_word, m);
    fclose(fp);
    return 0;
}

static int join_path(const char *a, const char *b, char *out, size_t cap) {
    size_t na = strlen(a), nb = strlen(b);
    int need_slash = (na > 0 && a[na - 1] != '/');
    size_t total = na + (need_slash ? 1 : 0) + nb + 1;
    if (total > cap) return 1;
    memcpy(out, a, na);
    size_t k = na;
    if (need_slash) out[k++] = '/';
    memcpy(out + k, b, nb);
    out[k + nb] = '\0';
    return 0;
}

static int scan_dir(wordmap_t *m, const char *path) {
    DIR *d = opendir(path);
    if (!d) return 1;

    int rc = 0;
    struct dirent *ent;
    char child[8192];

    while ((ent = readdir(d)) != NULL) {
        const char *name = ent->d_name;
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;

        if (join_path(path, name, child, sizeof(child)) != 0) { rc = 1; continue; }

        struct stat st;
        if (lstat(child, &st) != 0) { rc = 1; continue; }

        if (S_ISDIR(st.st_mode)) {
            rc |= scan_dir(m, child);
        } else if (S_ISREG(st.st_mode)) {
            rc |= scan_file(m, child);
        }
    }

    closedir(d);
    return rc;
}

int scan_path(wordmap_t *m, const char *path) {
    if (!m || !path) return 1;

    struct stat st;
    if (lstat(path, &st) != 0) return 1;

    if (S_ISDIR(st.st_mode)) return scan_dir(m, path);
    if (S_ISREG(st.st_mode)) return scan_file(m, path);
    return 0;
}

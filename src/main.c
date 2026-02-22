#include "pairio.h"
#include "walk.h"
#include "wordmap.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define ALPHABETLEN 26

static int cmp_wordcount(const void *a, const void *b) {
    const wordcount_t *x = (const wordcount_t *)a;
    const wordcount_t *y = (const wordcount_t *)b;
    return strcmp(x->word, y->word);
}

static void init_whichpipe(int *whichpipe, int nprocs) {
    // course-like: partition letters into contiguous blocks across reducers
    // whichpipe[letter_index] -> reducer index
    if (nprocs <= 0) nprocs = 1;
    for (int i = 0; i < ALPHABETLEN; i++) {
        // integer partition of [0,26) into nprocs buckets
        int r = (i * nprocs) / ALPHABETLEN;
        if (r < 0) r = 0;
        if (r >= nprocs) r = nprocs - 1;
        whichpipe[i] = r;
    }
}

static void scanner_send_map(wordmap_t *m, int nprocs, int whichpipe[ALPHABETLEN], int *reduce_wfds) {
    size_t n = 0;
    wordcount_t *arr = wordmap_export(m, &n);
    if (!arr) return;

    // Deterministic: send in sorted word order (matches "amap_getnext" vibe)
    qsort(arr, n, sizeof(wordcount_t), cmp_wordcount);

    for (size_t i = 0; i < n; i++) {
        const char *w = arr[i].word;
        if (!w || !*w) continue;
        int idx = (int)(w[0] - 'a');
        if (idx < 0 || idx >= ALPHABETLEN) continue;
        int r = whichpipe[idx];
        if (r < 0 || r >= nprocs) continue;
        (void)write_pair(reduce_wfds[r], w, arr[i].count);
    }
    free(arr);
}

static void run_scanner_child(const char *path, int nprocs, int whichpipe[ALPHABETLEN], int *reduce_wfds) {
    wordmap_t *m = wordmap_create();
    if (!m) _exit(2);

    (void)scan_path(m, path);
    scanner_send_map(m, nprocs, whichpipe, reduce_wfds);
    wordmap_free(m);

    for (int i = 0; i < nprocs; i++) close(reduce_wfds[i]);
    _exit(0);
}

static void run_reducer_child(int my_index, int nprocs, int my_readfd, int my_driver_wfd) {
    (void)my_index; (void)nprocs;

    wordmap_t *m = wordmap_create();
    if (!m) _exit(2);

    for (;;) {
        char *word = NULL;
        int64_t count = 0;
        int rr = read_pair(my_readfd, &word, &count);
        if (rr == 0) break;      // EOF
        if (rr < 0) break;       // error -> best-effort exit
        wordmap_add(m, word, count);
        free(word);
    }
    close(my_readfd);

    // Emit sorted results to driver
    size_t n = 0;
    wordcount_t *arr = wordmap_export(m, &n);
    if (arr) {
        qsort(arr, n, sizeof(wordcount_t), cmp_wordcount);
        for (size_t i = 0; i < n; i++) {
            (void)write_pair(my_driver_wfd, arr[i].word, arr[i].count);
        }
        free(arr);
    }

    wordmap_free(m);
    close(my_driver_wfd);
    _exit(0);
}

static void usage(const char *prog) {
    fprintf(stderr, "usage: %s <path> [<path> ...]\n", prog);
}

int main(int argc, char **argv) {
    if (argc < 2) { usage(argv[0]); return 2; }

    int nprocs = argc - 1; // course-like: reducers == scanners == number of input paths
    if (nprocs <= 0) nprocs = 1;

    int whichpipe[ALPHABETLEN];
    init_whichpipe(whichpipe, nprocs);

    // allocate pipes: one reduce pipe per reducer, one driver pipe per reducer
    int (*reducepipes)[2] = calloc((size_t)nprocs, sizeof(int[2]));
    int (*driverpipes)[2] = calloc((size_t)nprocs, sizeof(int[2]));
    if (!reducepipes || !driverpipes) { perror("calloc"); return 2; }

    for (int i = 0; i < nprocs; i++) {
        if (pipe(reducepipes[i]) != 0) { perror("pipe"); return 2; }
        if (pipe(driverpipes[i]) != 0) { perror("pipe"); return 2; }
    }

    // fork reducers
    pid_t *reducers = calloc((size_t)nprocs, sizeof(pid_t));
    if (!reducers) { perror("calloc"); return 2; }

    for (int i = 0; i < nprocs; i++) {
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); return 2; }
        if (pid == 0) {
            // reducer child: keep reducepipes[i][0] and driverpipes[i][1]
            for (int j = 0; j < nprocs; j++) {
                if (j == i) {
                    close(reducepipes[j][1]);   // close write end; keep read
                    close(driverpipes[j][0]);   // close read end; keep write
                } else {
                    close(reducepipes[j][0]);
                    close(reducepipes[j][1]);
                    close(driverpipes[j][0]);
                    close(driverpipes[j][1]);
                }
            }
            run_reducer_child(i, nprocs, reducepipes[i][0], driverpipes[i][1]);
        }
        reducers[i] = pid;
    }

    // fork scanners (one per argument)
    pid_t *scanners = calloc((size_t)nprocs, sizeof(pid_t));
    if (!scanners) { perror("calloc"); return 2; }

    for (int si = 0; si < nprocs; si++) {
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); return 2; }
        if (pid == 0) {
            // scanner child: keeps all reducepipes[*][1], closes reducepipes[*][0]
            int *wfds = calloc((size_t)nprocs, sizeof(int));
            if (!wfds) _exit(2);
            for (int i = 0; i < nprocs; i++) {
                close(reducepipes[i][0]);
                wfds[i] = reducepipes[i][1];
                // scanners don't use driverpipes
                close(driverpipes[i][0]);
                close(driverpipes[i][1]);
            }
            run_scanner_child(argv[si + 1], nprocs, whichpipe, wfds);
        }
        scanners[si] = pid;
    }

    // parent: doesn't use reducepipes, only reads driverpipes[*][0]
    for (int i = 0; i < nprocs; i++) {
        close(reducepipes[i][0]);
        close(reducepipes[i][1]);
        close(driverpipes[i][1]); // keep read end
    }

    // wait scanners so reducers eventually see EOF on their input pipes
    for (int i = 0; i < nprocs; i++) {
        int status = 0;
        (void)waitpid(scanners[i], &status, 0);
    }

    // Print reducer outputs in reducer-index order.
    // With letter partitioning, this yields overall alphabetical order.
    for (int i = 0; i < nprocs; i++) {
        for (;;) {
            char *word = NULL;
            int64_t count = 0;
            int rr = read_pair(driverpipes[i][0], &word, &count);
            if (rr == 0) break;
            if (rr < 0) break;
            printf("%s %" PRId64 "\n", word, count);
            free(word);
        }
        close(driverpipes[i][0]);
    }

    // wait reducers
    for (int i = 0; i < nprocs; i++) {
        int status = 0;
        (void)waitpid(reducers[i], &status, 0);
    }

    free(scanners);
    free(reducers);
    free(reducepipes);
    free(driverpipes);
    return 0;
}

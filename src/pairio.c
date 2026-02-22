#include "pairio.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int writen(int fd, const void *buf, size_t n) {
    const unsigned char *p = (const unsigned char *)buf;
    while (n > 0) {
        ssize_t w = write(fd, p, n);
        if (w < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        p += (size_t)w;
        n -= (size_t)w;
    }
    return 0;
}

static int readn(int fd, void *buf, size_t n) {
    unsigned char *p = (unsigned char *)buf;
    while (n > 0) {
        ssize_t r = read(fd, p, n);
        if (r == 0) return 0;          // EOF
        if (r < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        p += (size_t)r;
        n -= (size_t)r;
    }
    return 1;
}

int write_pair(int fd, const char *word, int64_t count) {
    if (!word) { errno = EINVAL; return -1; }
    uint32_t len = (uint32_t)strlen(word);
    if (writen(fd, &len, sizeof(len)) < 0) return -1;
    if (len > 0 && writen(fd, word, len) < 0) return -1;
    if (writen(fd, &count, sizeof(count)) < 0) return -1;
    return 1;
}

int read_pair(int fd, char **out_word, int64_t *out_count) {
    if (!out_word || !out_count) { errno = EINVAL; return -1; }

    uint32_t len = 0;
    int rr = readn(fd, &len, sizeof(len));
    if (rr == 0) return 0;
    if (rr < 0) return -1;

    char *w = (char *)malloc((size_t)len + 1);
    if (!w) return -1;

    if (len > 0) {
        rr = readn(fd, w, (size_t)len);
        if (rr <= 0) { free(w); return (rr == 0) ? 0 : -1; }
    }
    w[len] = '\0';

    int64_t c = 0;
    rr = readn(fd, &c, sizeof(c));
    if (rr <= 0) { free(w); return (rr == 0) ? 0 : -1; }

    *out_word = w;
    *out_count = c;
    return 1;
}

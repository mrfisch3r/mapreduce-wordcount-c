#include "tokenize.h"

#include <ctype.h>

#define WORD_MAX 4000

void tokenize_stream(FILE *fp, word_cb cb, void *ctx) {
    if (!fp || !cb) return;

    char buf[WORD_MAX + 1];
    size_t n = 0;

    for (;;) {
        int ch = fgetc(fp);
        if (ch == EOF) {
            if (n > 0) {
                buf[n] = '\0';
                cb(buf, ctx);
            }
            return;
        }

        if (isalpha((unsigned char)ch)) {
            if (n < WORD_MAX) {
                buf[n++] = (char)tolower((unsigned char)ch);
            } else {
                // truncate: keep consuming until delimiter
            }
        } else {
            if (n > 0) {
                buf[n] = '\0';
                cb(buf, ctx);
                n = 0;
            }
        }
    }
}

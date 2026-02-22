#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct wordmap wordmap_t;

wordmap_t *wordmap_create(void);
void wordmap_free(wordmap_t *m);

void wordmap_add(wordmap_t *m, const char *word, int64_t delta);
size_t wordmap_size(const wordmap_t *m);

typedef struct {
    const char *word;   // pointer owned by map
    int64_t count;
} wordcount_t;

wordcount_t *wordmap_export(const wordmap_t *m, size_t *out_n);

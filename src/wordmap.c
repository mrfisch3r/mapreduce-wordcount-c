#include "wordmap.h"

#include <stdlib.h>
#include <string.h>

typedef struct node {
    char *word;
    int64_t count;
    struct node *next;
} node_t;

struct wordmap {
    node_t **buckets;
    size_t nbuckets;
    size_t size;
};

static uint64_t fnv1a(const char *s) {
    uint64_t h = 1469598103934665603ULL;
    for (const unsigned char *p = (const unsigned char*)s; *p; p++) {
        h ^= (uint64_t)(*p);
        h *= 1099511628211ULL;
    }
    return h;
}

static char *xstrdup(const char *s) {
    size_t n = strlen(s) + 1;
    char *p = (char *)malloc(n);
    if (!p) return NULL;
    memcpy(p, s, n);
    return p;
}

static void free_nodes(node_t *n) {
    while (n) {
        node_t *next = n->next;
        free(n->word);
        free(n);
        n = next;
    }
}

static void wordmap_rehash(wordmap_t *m, size_t new_nb) {
    node_t **newb = (node_t **)calloc(new_nb, sizeof(node_t *));
    if (!newb) return;

    for (size_t i = 0; i < m->nbuckets; i++) {
        node_t *cur = m->buckets[i];
        while (cur) {
            node_t *next = cur->next;
            size_t idx = (size_t)(fnv1a(cur->word) % new_nb);
            cur->next = newb[idx];
            newb[idx] = cur;
            cur = next;
        }
    }

    free(m->buckets);
    m->buckets = newb;
    m->nbuckets = new_nb;
}

wordmap_t *wordmap_create(void) {
    wordmap_t *m = (wordmap_t *)calloc(1, sizeof(wordmap_t));
    if (!m) return NULL;
    m->nbuckets = 1024;
    m->buckets = (node_t **)calloc(m->nbuckets, sizeof(node_t *));
    if (!m->buckets) { free(m); return NULL; }
    return m;
}

void wordmap_free(wordmap_t *m) {
    if (!m) return;
    for (size_t i = 0; i < m->nbuckets; i++) {
        free_nodes(m->buckets[i]);
    }
    free(m->buckets);
    free(m);
}

size_t wordmap_size(const wordmap_t *m) {
    return m ? m->size : 0;
}

void wordmap_add(wordmap_t *m, const char *word, int64_t delta) {
    if (!m || !word || !*word) return;

    if (m->size > m->nbuckets * 2) {
        wordmap_rehash(m, m->nbuckets * 2);
    }

    size_t idx = (size_t)(fnv1a(word) % m->nbuckets);

    for (node_t *n = m->buckets[idx]; n; n = n->next) {
        if (strcmp(n->word, word) == 0) {
            n->count += delta;
            return;
        }
    }

    node_t *nn = (node_t *)calloc(1, sizeof(node_t));
    if (!nn) return;
    nn->word = xstrdup(word);
    if (!nn->word) { free(nn); return; }
    nn->count = delta;
    nn->next = m->buckets[idx];
    m->buckets[idx] = nn;
    m->size++;
}

wordcount_t *wordmap_export(const wordmap_t *m, size_t *out_n) {
    if (out_n) *out_n = 0;
    if (!m) return NULL;

    wordcount_t *arr = (wordcount_t *)calloc(m->size, sizeof(wordcount_t));
    if (!arr) return NULL;

    size_t k = 0;
    for (size_t i = 0; i < m->nbuckets; i++) {
        for (node_t *n = m->buckets[i]; n; n = n->next) {
            arr[k].word = n->word;
            arr[k].count = n->count;
            k++;
        }
    }
    if (out_n) *out_n = k;
    return arr;
}

#pragma once
#include <stdio.h>

typedef void (*word_cb)(const char *word, void *ctx);

void tokenize_stream(FILE *fp, word_cb cb, void *ctx);

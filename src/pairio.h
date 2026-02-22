#pragma once
#include <stdint.h>

int write_pair(int fd, const char *word, int64_t count);
/* Returns:
 *  1 on success,
 *  0 on clean EOF,
 * -1 on error (errno set)
 */
int read_pair(int fd, char **out_word, int64_t *out_count);

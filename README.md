# MapReduce-style Word Count (C)

A portfolio-safe, standalone reimplementation of a coursework-style systems programming project:
a **multi-process** word counter in C using **fork + pipes + wait**, with directory recursion and reducer aggregation.

This repository contains only code and documentation written by me (no course handouts/prompts or instructor-provided tests/corpora).

## Behavior (course-like)
- Run: `./wordcount <path> [<path> ...]`
- **One scanner process per input path**
- **One reducer process per input path** (same count as scanners)
- Scanners tokenize files into words, count locally, then send `(word,count)` pairs to reducers based on the first letter.
- Reducers aggregate counts and send results back to the driver.
- The driver prints final results sorted alphabetically as:

```
word count
```

## Build & run (Linux / WSL)
```bash
make
./wordcount samples
```

## Run tests
```bash
make
./tests/test_small.sh
./tests/test_multiargs.sh
```

## Notes / design choices
- Tokenization rule: words are **alphabetic runs** (`isalpha`), converted to lowercase.
- Pipe message framing sends `(word, count)` pairs.
- Values are `int64_t` to avoid overflow for larger corpora.

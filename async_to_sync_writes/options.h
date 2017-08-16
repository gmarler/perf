#ifndef OPTIONS_H
#define OPTIONS_H
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

void usage(char **argv);
int
collect_options(char **argv, char *filepath, long long *filesize,
                long long *blocksize,
                enum test_type *test, int *sync_type);

#endif /* OPTIONS_H */


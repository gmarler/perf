#ifndef OPTIONS_H
#define OPTIONS_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>

void usage(char **argv);
int
collect_options(int *argc, char **argv, char *filepath,
                long long *filesize,
                long long *blocksize,
                enum test_type *test, int *sync_type,
                long long *rename_delay);

#endif /* OPTIONS_H */



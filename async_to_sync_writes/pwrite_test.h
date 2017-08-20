#ifndef PWRITE_TEST_H
#define PWRITE_TEST_H

#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

void pwrite_test(int fd, long long filesize, long long blocksize,
                 char *buffers, int buffer_count);

#endif /* PWRITE_TEST_H */

#ifndef LIO_LISTIO_TEST_H
#define LIO_LISTIO_TEST_H

#include <aio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

void lio_listio_test(int fd, long long filesize, long long blocksize,
                     char *buffers, int buffer_count);

#endif /* LIO_LISTIO_TEST_H */

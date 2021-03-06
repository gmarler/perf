#ifndef LIO_LISTIO_TEST_H
#define LIO_LISTIO_TEST_H

#include <aio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <stdbool.h>
#include "my_signals.h"

void lio_listio_test(int fd, long long filesize, long long blocksize,
                     char *buffers, int buffer_count);

static void *sig_thread(void *arg);
static void  io_completion_handler();

#endif /* LIO_LISTIO_TEST_H */

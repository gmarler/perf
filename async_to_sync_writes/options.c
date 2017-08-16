/******************************************************************************
 * Options for this test:
 * --filepath=/path/to/file
 *  Path to the file you want this test to write out
 * --filesize=<size in bytes>
 *  Total size of the file this test will create
 * --blocksize=<size in bytes>
 *  Size of each write to the file
 * --test=(pwrite|aio_write|lio_listio)
 *  The way to perform writes - synchronous or asynchronous
 * --sync_type=(O_SYNC|O_DSYNC|<none - async - default)
 *  Whether I/O is synchronized or non-synchronized (default)
 ******************************************************************************/

enum test_type { Test_pwrite, Test_aio_write, Test_lio_listio };

#ifndef _OPTIONS_H_
#define _OPTIONS_H_
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

void usage(char **argv);
int
collect_options(char **argv, char *filepath, long long *filesize,
                long long *blocksize,
                enum test_type *test, int *sync_type);

#endif /* _OPTIONS_H_ */


int
collect_options(char **argv, char *filepath, long long *filesize,
                long long *blocksize,
                enum test_type *test, int *sync_type)
{
  static struct option long_options[] =
  {
    {"filepath",  required_argument, 0, 'f'},
    {"filesize",  required_argument, 0, 's'},
    {"blocksize", required_argument, 0, 'b'},
    {"test",      required_argument, 0, 't'},
    {"sync_type", required_argument, 0, 'y'},
    {0,                           0, 0,   0}
  };
}

void usage(char **argv)
{
  printf("Usage: %s\n",argv[0]);
  printf("--filepath=</path/to/file>\n");
  printf("  Path to file to write\n");
  printf("--filesize=<size in bytes>\n");
  printf("  Size of the file you want to create\n");
  printf("--blocksize=<size in bytes>\n");
  printf("  Each I/O to the file will be of this size\n");
  printf("--test=(pwrite|aio_write|lio_listio)\n");
  printf("  synchronous or asynchronous write type\n");
  printf("--sync_type=(O_SYNC|O_DSYNC|<none - default>)\n");
  printf("  synchronized or non-synchronized I/Os\n");

  exit(0);
}

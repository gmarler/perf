#include <stdio.h>
#include <limits.h>
#include "test_type.h"
#include "options.h"
#include "buffer_initialize.h"

/* the number of buffers filled with random data, which we choose from randomly
 * to write the destination file */
#define BUFFERS  20

int main(int argc, char **argv)
{
  char            filepath[PATH_MAX];
  long long       filesize;
  long long       blocksize;
  enum test_type  test;
  int             sync_type;
  char           *buffers;

  collect_options(&argc, argv, filepath, &filesize, &blocksize, &test, &sync_type);

  printf("Running with following options:\n");
  printf("File to write: %s, File size %lld, I/O Block Size: %lld\n",
         filepath, filesize, blocksize);
  printf("I/O Test: %s, Synchronized Type: %s\n",
         test == Test_pwrite ? "pwrite" :
         test == Test_aio_write ? "aio_write" :
         test == Test_lio_listio ? "lio_listio" : "UNKNOWN",
         sync_type & O_SYNC ? "O_SYNC" :
         sync_type & O_DSYNC ? "O_DSYNC" :
         "No Synchronization");

  /* Fill buffers with random data */
  buffer_initialize(buffers, BUFFERS, blocksize);

  /* Open file to write to with proper flags */
  /* Initiate the write test activity */
}

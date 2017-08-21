#include <stdio.h>
#include <limits.h>
#include <time.h>
#include "test_type.h"
#include "options.h"
#include "buffer_initialize.h"
#include "pwrite_test.h"

/* the number of buffers filled with random data, which we choose from randomly
 * to write the destination file */
#define BUFFER_COUNT  20

int main(int argc, char **argv)
{
  char            filepath[PATH_MAX];
  long long       filesize;
  long long       blocksize;
  enum test_type  test;
  int             sync_type = 0;
  char           *buffers;
  time_t          t;
  struct tm      *tm;
  char            timestamp[64];

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
  buffer_initialize(&buffers, BUFFER_COUNT, blocksize);
  printf("INITIALIZED BUFFER BASE ADDRESS: %lld\n",buffers);

  /* Open file to write to with proper flags */
  /* Opening "synchronized" (O_DSYNC), and O_APPEND, so we don't have to specify
     the offset in the aio control block */
  int open_flags = O_RDWR | O_CREAT | O_APPEND | sync_type;
  int fd = open(filepath, open_flags, 0755);
  if (fd == NULL) {
    perror("Unable to open file");
    exit(1);
  }

  /* Initiate the write test activity */
  t = time(NULL);
  tm = localtime(&t);
  strftime(timestamp, sizeof(timestamp), "%c", tm);
  printf("    Writing begins at: %s\n",timestamp);
  if (test == Test_pwrite) {
    pwrite_test(fd, filesize, blocksize, buffers, BUFFER_COUNT);
  } else if (test == Test_aio_write) {

  } else if (test == Test_lio_listio) {

  }

  /* Rename file */
  t = time(NULL);
  tm = localtime(&t);
  strftime(timestamp, sizeof(timestamp), "%c", tm);
  printf("     Renaming file at: %s\n",timestamp);

  t = time(NULL);
  tm = localtime(&t);
  strftime(timestamp, sizeof(timestamp), "%c", tm);
  printf(" Renaming complete at: %s\n",timestamp);
}

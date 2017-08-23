#include <stdio.h>
#include <limits.h>
#include <time.h>
#include <unistd.h>
#include "test_type.h"
#include "options.h"
#include "buffer_initialize.h"
#include "pwrite_test.h"
#include "lio_listio_test.h"

/* the number of buffers filled with random data, which we choose from randomly
 * to write the destination file */
#define BUFFER_COUNT  20

int main(int argc, char **argv)
{
  char            filepath[PATH_MAX];
  char           *filepath_renamed;
  char            filepath_suffix[] = ".done";
  long long       filesize;
  long long       blocksize;
  long long       rename_delay = 0; /* default to 0 */
  enum test_type  test;
  int             sync_type = 0;
  char           *buffers;
  time_t          t;
  struct tm      *tm;
  char            timestamp[64];

  collect_options(&argc, argv, filepath, &filesize, &blocksize, &test, &sync_type,&rename_delay);

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
  printf("Rename delay after I/Os submitted: %lld seconds\n",rename_delay);

  /* Set up the renamed file name */
  filepath_renamed = malloc(sizeof(filepath) + sizeof(filepath_suffix));
  strcat(filepath_renamed,filepath);
  strcat(filepath_renamed,filepath_suffix);

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
    lio_listio_test(fd, filesize, blocksize, buffers, BUFFER_COUNT);
  }

  /* Rename file */
  t = time(NULL);
  tm = localtime(&t);
  strftime(timestamp, sizeof(timestamp), "%c", tm);
  if (rename_delay) {
    printf("Sleeping %lld seconds before rename...\n",rename_delay);
    sleep(rename_delay);
  }
  printf("     Renaming file %s to %s at: %s\n",filepath,filepath_renamed,
                                                timestamp);
  rename(filepath,filepath_renamed);
  t = time(NULL);
  tm = localtime(&t);
  strftime(timestamp, sizeof(timestamp), "%c", tm);
  printf(" Renaming complete at: %s\n",timestamp);
  printf("sleeping 4 seconds after rename and before unlink...\n");
  sleep(4);
  unlink(filepath_renamed);
}

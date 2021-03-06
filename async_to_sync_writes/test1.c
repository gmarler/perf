/*
 * Test how a large number of 8KB async writes, suddenly fsync()'ed, then
 * close()'d, will perform on ZFS
 * */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFSIZE 8192
#define BUFFERS   16

/* Declarations */
long random_at_most(long max);

int
main(int argc, char **argv)
{
  int i;
  const char filename[] = "/perfwork/bogus-file";
  char buffers[BUFFERS][BUFSIZE];
  long buffer_number;
  char *buffer;

  printf("Generating random buffers\n");
  int randfd = open("/dev/urandom", O_RDONLY);
  if (randfd == NULL) {
    perror("Unable to open /dev/urandom");
    exit(2);
  }
  for (i = 0; i < BUFFERS; i++) {
    read(randfd,buffers[i],BUFSIZE);
  }

  int fd = open(filename, O_RDWR | O_CREAT, 0755);

  if (fd == NULL) {
    perror("Unable to open file");
    exit(1);
  }

  /* size of the file we want in bytes */
  unsigned long long file_size  = 4L * 1024L * 1024L * 1024L;
  unsigned long iterations = file_size / BUFSIZE;

  printf("Writing random data to file\n");
  srand(time(NULL));
  while (iterations--) {
    /* buffer_number = random_at_most(BUFFERS - 1); */
    buffer_number = ( rand() % BUFFERS );
    write(fd, buffers[buffer_number], BUFSIZE);
  }
  fdatasync(fd);
  close(fd);
}

// Assumes 0 <= max <= RAND_MAX
// Returns in the closed interval [0, max]
long random_at_most(long max) {
  unsigned long
    // max <= RAND_MAX < ULONG_MAX, so this is okay.
    num_bins = (unsigned long) max + 1,
    num_rand = (unsigned long) RAND_MAX + 1,
    bin_size = num_rand / num_bins,
    defect   = num_rand % num_bins;

  long x;
  do {
   x = random();
  }
  // This is carefully written not to overflow
  while (num_rand - defect <= (unsigned long)x);

  // Truncated division is intentional
  return x/bin_size;
}

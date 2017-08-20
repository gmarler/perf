#include "pwrite_test.h"

void pwrite_test(int fd, long long filesize, long long blocksize,
                 char *buffers, int buffer_count)
{
  unsigned long  iterations = filesize / blocksize;
  long           buffer_number;
  char          *buffer;
  off_t          offset;
  ssize_t        written;

  printf("ITERATIONS: %lu\n",iterations);
  printf("BUFFER ADDRESS RANGE STARTS AT: %lld\n", buffers);

  srand(time(NULL));

  for (int i = 0; i < iterations; i++) {
    offset = i * blocksize;
    buffer_number = ( rand() % buffer_count );
    printf("BUFFER %d picked\n",buffer_number);
    buffer = buffers + (buffer_number * blocksize);
    written = pwrite(fd, buffer, blocksize, offset);
    exit(1);
    if (written == -1) {
      printf("Failed to write iteration %d\n", i);
      perror("FAILED WITH");
    }
  }
}

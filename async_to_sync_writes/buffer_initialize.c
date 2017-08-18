#include "buffer_initialize.h"

void buffer_initialize(char *buffers,int buffer_count, int bufsize)
{
  int   i;
  long  buffer_number;
  char *buffer;

  /* Allocate memory for buffers */
  buffers = malloc(buffer_count * bufsize);

  /* Fill buffers with random data */
  printf("Generating random buffers\n");
  int randfd = open("/dev/urandom", O_RDONLY);
  if (randfd == NULL) {
    perror("Unable to open /dev/urandom");
    exit(2);
  }
  printf("        Base address %lld\n",buffers);
  for (i = 0; i < buffer_count; i++) {
    printf("Reading into address %lld\n",(buffers+(i*bufsize)));
    read(randfd,(buffers+(i*bufsize)),bufsize);
  }

}

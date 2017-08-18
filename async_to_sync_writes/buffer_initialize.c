#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

void buffer_initialize(char *buffers,int buffers, int bufsize)
{
  int   i;
  long  buffer_number;
  char *buffer;

  /* Allocate memory for buffers */
  buffers = malloc(buffers * bufsize);

  /* Fill buffers with random data */
  printf("Generating random buffers\n");
  int randfd = open("/dev/urandom", O_RDONLY);
  if (randfd == NULL) {
    perror("Unable to open /dev/urandom");
    exit(2);
  }
  for (i = 0; i < buffers; i++) {
    read(randfd,buffers[i],bufsize);
  }

}

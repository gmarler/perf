/*
 *
 * For this test, we will be doing the following:
 * - Opening a file
 * - Performing "asynchronous" writes to it via lio_listio(LIO_WAIT,...)
 *
 * */

#include <iostream>
#include <string>
#include <vector>
#include <aio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <cstdlib>
#include <unistd.h>

using std::vector;

#define BUFSIZE 8192
#define BUFFERS   16

int main()
{
  int                            i;
  aiocb_t           *control_block;
  vector<aiocb_t *>  control_blocks;

  std::cout << "BEGIN" << std::endl;

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

  /* Opening "synchronized" (O_DSYNC), and O_APPEND, so we don't have to specify
   * the offset in the aio control block */
  // int fd = open(filename, O_RDWR | O_CREAT | O_APPEND | O_DSYNC, 0755);
  int fd = open(filename, O_RDWR | O_CREAT | O_APPEND, 0755);

  if (fd == NULL) {
    perror("Unable to open file");
    exit(1);
  }

  /* size of the file we want in bytes */
  unsigned long long file_size  = 4L * 1024L * 1024L * 1024L;
  unsigned long iterations = file_size / BUFSIZE;

  std::cout << "Writing random data to file" << std::endl;
  srand(time(NULL));

  for (int i = 0; i < iterations; i += 18) {
    // lio_listio only supports a certain number of aiocb's - 18 in this case
    for (int j = 0; j < 18; j++) {

      /* buffer_number                         = random_at_most(BUFFERS - 1); */
      buffer_number                            = ( rand() % BUFFERS );

      control_block                            = (aiocb_t *)malloc(sizeof(aiocb_t));

      control_block->aio_fildes                = fd;
      control_block->aio_offset                = 0;
      control_block->aio_sigevent.sigev_notify = SIGEV_NONE;
      control_block->aio_nbytes                = BUFSIZE;
      control_block->aio_buf                   = buffers[buffer_number];
      control_block->aio_reqprio               = 0;
      /* Using lio_listio() here */
      control_block->aio_lio_opcode            = LIO_WRITE;

      control_blocks.push_back(control_block);
    }
    int ret = lio_listio(LIO_WAIT,&control_blocks[0],(int)control_blocks.size(),NULL);
    if (ret == 0) {
      // std::cout << "lio_listio SUCCESSFUL" << std::endl;
    } else {
      perror("lio_listio FAILED");
      return(1);
    }
    // clear control_blocks vector
    control_blocks.clear();
  }
  // Iterate over vector of control blocks, checking that each is complete
  // before closing
  // Note that this isn't strictle necessary, and slows us down a bit
  std::cout << "CONFIRMING I/O made it to disk" << std::endl;
  close(fd);


  return(0);
}

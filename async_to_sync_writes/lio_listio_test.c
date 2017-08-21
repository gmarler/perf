#include "lio_listio_test.h"

void lio_listio_test(int fd, long long filesize, long long blocksize,
                     char *buffers, int buffer_count)
{
  unsigned long      iterations = filesize / blocksize;
  long               buffer_number;
  char              *buffer;
  aiocb_t           *control_block;
  aiocb_t           *control_blocks;
  long               aio_listio_max;

  /* Determine max number AIOs that can be initiated in a single lio_listio()
   * call on the platform */
  aio_listio_max = sysconf(_SC_AIO_LISTIO_MAX);
  printf("On this platform, AIO_LISTIO_MAX = [%ld]\n",aio_listio_max);
  printf("Max outstanding AIO I/Os: [%ld]\n",sysconf(_SC_AIO_MAX));

  /* Disable reception of SIGRTMAX/SIGRTMAX-1 in the main thread */

  /* Start the I/O completion handling thread */
  /* Enable reception of SIGRTMAX/SIGRTMAX-1 in the I/O completion handling
   * thread */
  srand(time(NULL));

  for (int i = 0; i < iterations; i += aio_listio_max) {
    // lio_listio only supports a certain number of aiocb's - 18 in this case
    for (int j = 0; j < aio_listio_max; j++) {

      /* buffer_number                         = random_at_most(BUFFERS - 1); */
      buffer_number                            = ( rand() % buffer_count );

      control_block                            = (aiocb_t *)malloc(sizeof(aiocb_t));

      control_block->aio_fildes                = fd;
      control_block->aio_offset                = 0;
      /* Notify via signal */
      control_block->aio_sigevent.sigev_notify = SIGEV_NONE;
      control_block->aio_nbytes                = blocksize;
      control_block->aio_buf                   = buffers + (buffer_number * blocksize);
      control_block->aio_reqprio               = 0;
      /* Using lio_listio() here */
      control_block->aio_lio_opcode            = LIO_WRITE;

      control_blocks[j] = control_block;
    }
    int ret = lio_listio(LIO_WAIT,control_blocks,(int)aio_listio_max,NULL);
    /* Wait for I/O completion handling thread to assert the condition variable
     * to continue on */

    if (ret == 0) {
    } else {
      perror("lio_listio FAILED");
      return(1);
    }
    // clear control_blocks vector
    control_blocks.clear();
  }


  for (int i = 0; i < iterations; i++) {
    offset = i * blocksize;
    buffer_number = ( rand() % buffer_count );
    /*  printf("BUFFER %d picked\n",buffer_number); */
    buffer = buffers + (buffer_number * blocksize);
    written = pwrite(fd, buffer, blocksize, offset);
    if (written == -1) {
      printf("Failed to write iteration %d\n", i);
      perror("FAILED WITH");
    }
  }
}

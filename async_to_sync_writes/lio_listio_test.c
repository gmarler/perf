#include "lio_listio_test.h"

void lio_listio_test(int fd, long long filesize, long long blocksize,
                     char *buffers, int buffer_count)
{
  unsigned long      iterations = filesize / blocksize;
  long               buffer_number;
  char              *buffer;
  struct aiocb      *control_block;
  long               aio_listio_max;
  sigset_t           set;
  struct sigaction   act;
  pthread_t          tid;
  struct sigevent    per_io_list_sigevent;

  /* Determine max number AIOs that can be initiated in a single lio_listio()
   * call on the platform */
  aio_listio_max = sysconf(_SC_AIO_LISTIO_MAX);
  /*
  if (aio_listio_max > 1024) {
    aio_listio_max = 1024;
  }
  */
  printf("On this platform, AIO_LISTIO_MAX = [%ld]\n",aio_listio_max);
  printf("Max outstanding AIO I/Os: [%ld]\n",sysconf(_SC_AIO_MAX));

  /* don't know how long to make the array until you determine aio_listio_max */
  struct aiocb      *control_blocks[aio_listio_max];

  /* Disable reception of MYSIG_AIO_COMPLETE/MYSIG_STOP in the main thread */
  sigemptyset(&set);
  sigaddset(&set,MYSIG_AIO_COMPLETE);
  sigaddset(&set,MYSIG_STOP);
  pthread_sigmask(SIG_BLOCK, &set, NULL);

  /* Clear out the sigaction */
  memset(&act,0,sizeof(act));
  act.sa_flags = SA_SIGINFO;
  act.sa_sigaction = io_completion_handler;

  /* Start the I/O completion handling thread */
  pthread_create(&tid,NULL,sig_thread,&set);

  per_io_list_sigevent.sigev_notify          = SIGEV_SIGNAL;
  per_io_list_sigevent.sigev_signo           = MYSIG_AIO_COMPLETE;
  per_io_list_sigevent.sigev_value.sival_ptr = (void *)&control_blocks;

  /* Enable reception of SIGRTMAX/SIGRTMAX-1 in the I/O completion handling
   * thread */
  srand(time(NULL));

  for (int i = 0; i < iterations; i += aio_listio_max) {
    for (int j = 0; j < aio_listio_max; j++) {

      /* buffer_number                         = random_at_most(BUFFERS - 1); */
      buffer_number                            = ( rand() % buffer_count );

      control_block                            = (struct aiocb *)malloc(sizeof(struct aiocb));

      control_block->aio_fildes                = fd;
      control_block->aio_offset                = 0;
      /* Don't bother signal notification per aiocb, just get one signal when
       * they're all done */
      control_block->aio_sigevent.sigev_notify = SIGEV_NONE;
      control_block->aio_nbytes                = blocksize;
      control_block->aio_buf                   = buffers + (buffer_number * blocksize);
      control_block->aio_reqprio               = 0;
      /* Using lio_listio() here */
      control_block->aio_lio_opcode            = LIO_WRITE;

      control_blocks[j]                        = control_block;
    }

    /*int ret = lio_listio(LIO_WAIT,control_blocks,(int)aio_listio_max,NULL); */
    int ret = lio_listio(LIO_NOWAIT,control_blocks,(int)aio_listio_max,
                         &per_io_list_sigevent);

    /* When using LIO_NOWAIT above, Wait for I/O completion handling thread to
     * assert the condition variable to continue on */
    sleep(2);

    if (ret == 0) {
    } else {
      perror("lio_listio FAILED");
      /* return(1); */
    }
    /* clear control_blocks vector */
    /* control_blocks.clear(); */
    for (int i = 0;  i < aio_listio_max; i++) {
      /* TODO: Can't we just reuse the aiocbs?  */
      free(control_blocks[i]);
    }
  }

  /* Join with the I/O handling thread when we're done */
  pthread_join(tid,NULL);
}

static void io_completion_handler()
{

}

static void *sig_thread(void *arg)
{
  int         signum;
  siginfo_t   info;
  static long count = 0;

  do {
    signum = sigwaitinfo((sigset_t *)arg, &info);
    if (signum == MYSIG_AIO_COMPLETE) {
      /* cast needed: (struct aiocb *)info.si_value.sival_ptr */
      count++;
      printf("%ld I/O list completions handled\n",count);
    } else if (signum == MYSIG_STOP) {
      return (void *)true;
    }
  } while (signum != -1 || errno == EINTR);

}

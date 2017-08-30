#include "aio_write_test.h"

typedef struct {
  pthread_mutex_t    mutex;
  pthread_cond_t     condvar;
  struct aiocb      *control_blocks;
  /* the semantics here can be odd:
   * - If O_SYNC/O_DSYNC set on file descriptor, this means the I/O made it to
   *   stable storage
   * - If the above not set, this just means the I/O has been handed off to the
   *   kernel, no more
   */
  long               max_aios;
  long               ios_completed_per_batch;
  long               total_ios_completed;
  long               total_ios_needed;
  sigset_t           set;
} global_data_t;

typedef struct {
  global_data_t  *global_info;
  struct aiocb   *cblock;
} per_io_data_t;

void aio_write_test(int fd, long long filesize, long long blocksize,
                    char *buffers, int buffer_count)
{
  int                lock_status;
  unsigned long      iterations = filesize / blocksize;
  long               buffer_number;
  char              *buffer;
  struct aiocb      *control_block;
  sigset_t           set;
  pthread_t          tid;
  long               max_aios;
  global_data_t      central_global_data;

  /* Initialize mutex and condition variable
   * WARNING: this is a one time initialization - attempting twice will cause an
   *          error */
  pthread_mutex_init(&central_global_data.mutex,NULL);
  pthread_cond_init(&central_global_data.condvar,NULL);
  central_global_data.ios_completed_per_batch = 0; /* initialize to 0 */
  central_global_data.total_ios_completed     = 0; /* initialize to 0 */
  central_global_data.total_ios_needed = iterations;

  max_aios = sysconf(_SC_AIO_MAX);
  if (max_aios == -1) {
    /* it's undefined, so select a sane default */
    max_aios = 512;
  }
  printf("Max outstanding AIO I/Os: [%ld]\n",max_aios);

  /* don't know how long to make the array until you determine max_aios */
  struct aiocb      *control_blocks[max_aios];

  /* Use the same signal set to:
   * A. Disable reception of MYSIG_AIO_COMPLETE/MYSIG_STOP in the main thread
   * B. Enable reception of the same signals in the child thread via sigwaitinfo */
  sigemptyset(&set);
  sigaddset(&set,MYSIG_AIO_COMPLETE);
  sigaddset(&set,MYSIG_STOP);
  pthread_sigmask(SIG_BLOCK, &set, NULL);
  /* squirrel awway the signal set to be used with sigwaitinfo() in the child
   * thread */
  central_global_data.set      = set;
  /* Note when the signal handling thread can exit - when all I/Os have been
   * completed. */
  central_global_data.max_aios = max_aios;

  /* Start the I/O completion handling thread */
  pthread_create(&tid,NULL,sig_thread,&central_global_data);

  /*
  per_io_list_sigevent.sigev_notify          = SIGEV_SIGNAL;
  per_io_list_sigevent.sigev_signo           = MYSIG_AIO_COMPLETE;
  per_io_list_sigevent.sigev_value.sival_ptr = (void *)&control_blocks;
  */

  /* Enable reception of SIGRTMAX/SIGRTMAX-1 in the I/O completion handling
   * thread */
  srand(time(NULL));

  for (int i = 0; i < iterations; i += max_aios) {
    for (int j = 0; j < max_aios; j++) {

      /* buffer_number                         = random_at_most(BUFFERS - 1); */
      buffer_number                            = ( rand() % buffer_count );

      control_block                            = (struct aiocb *)malloc(sizeof(struct aiocb));

      control_block->aio_fildes                = fd;
      control_block->aio_offset                = 0;
      /* Don't bother signal notification per aiocb, just get one signal when
       * they're all done */
      control_block->aio_sigevent.sigev_notify = SIGEV_SIGNAL;
      control_block->aio_sigevent.sigev_signo  = MYSIG_AIO_COMPLETE;
      control_block->aio_nbytes                = blocksize;
      control_block->aio_buf                   = buffers + (buffer_number * blocksize);
      control_block->aio_reqprio               = 0;

      control_blocks[j]                        = control_block;

      /* Check individual I/O submission (not completion) status here */
      int ret = aio_write(control_block);
      if (ret == 0) {
      } else {
        perror("aio_write() initiation FAILED");
        /* return(1); */
      }

    }


    /* Wait for I/O completion handling thread to assert the condition
     * variable to continue on */
    lock_status = pthread_mutex_lock(&central_global_data.mutex);
    if (lock_status != 0) {
      perror("Unable to do initial mutex lock");
      exit(1);
    }
    while (central_global_data.total_ios_completed %
           central_global_data.max_aios == 0) {
      lock_status = pthread_cond_wait(&central_global_data.condvar,
                                      &central_global_data.mutex);
      if (lock_status != 0) {
        perror("Condition wait failed");
        exit(2);
      }
    }

    if (central_global_data.total_ios_completed %
        central_global_data.max_aios == 0)
      printf("I/O batch condition signaled\n");

    lock_status = pthread_mutex_unlock(&central_global_data.mutex);
    if (lock_status != 0) {
      perror("Unable to do final mutex unlock");
      exit(1);
    }

    /* clear control_blocks vector */
    /* control_blocks.clear(); */
    for (int i = 0;  i < max_aios; i++) {
      /* TODO: we can reuse the aiocbs, so just clear them here  */
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
  int            lock_status;
  int            signum;
  siginfo_t      info;
  global_data_t *global_data = (global_data_t *)arg;
  struct aiocb  *my_aiocb;

  printf("Entered sig_thread\n");
  do {
    signum = sigwaitinfo(&(global_data->set), &info);
    if (signum == MYSIG_AIO_COMPLETE) {
      /* cast needed: (global_data_t *)info.si_value.sival_ptr */
      my_aiocb = (struct aiocb *)info.si_value.sival_ptr;
      global_data->ios_completed_per_batch++;
      lock_status = pthread_mutex_lock(&(global_data->mutex));
      if (lock_status != 0) {
        perror("Unable to lock total_ios_completed");
        exit(3);
      }
      global_data->total_ios_completed++;
      lock_status = pthread_mutex_unlock(&(global_data->mutex));
      if (lock_status != 0) {
        perror("Unable to UNLOCK total_ios_completed");
        exit(4);
      }
      if (global_data->ios_completed_per_batch >= global_data->max_aios) {
        printf("One batch of aio_write() activities completed\n");
        printf("%ld aio_write completions handled\n",global_data->total_ios_completed);
        global_data->ios_completed_per_batch = 0; /* reset */
        lock_status = pthread_cond_signal(&(global_data->condvar));
        if (lock_status != 0) {
          perror("Unable to signal an INTERIM condition");
          exit(8);
        }
      }
      if (global_data->total_ios_completed >= global_data->total_ios_needed)
        break;
    } else if (signum == MYSIG_STOP) {
      /* Probably not needed for this scenario anyway */
      return (void *)true;
    }
  } while (signum != -1 || errno == EINTR);


  lock_status = pthread_mutex_lock(&(global_data->mutex));
  if (lock_status != 0) {
    perror("Unable to lock total_ios_completed");
    exit(5);
  }
  lock_status = pthread_cond_signal(&(global_data->condvar));
  if (lock_status != 0) {
    perror("Unable to signal FINAL condition");
    exit(7);
  }
  lock_status = pthread_mutex_unlock(&(global_data->mutex));
  if (lock_status != 0) {
    perror("Unable to UNLOCK total_ios_completed");
    exit(6);
  }

  return (void *)true;
}

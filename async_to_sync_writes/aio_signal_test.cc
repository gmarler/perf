/*
 * For this test, we will be doing the following:
 * - Opening a file "synchronized" (O_DSYNC)
 * - But performing "asynchronous" writes to it (aio_write)
 * - Creating a bitmap for all of the outgoing writes, initialized to all zeros
 * - Picking a signal to inform us that each I/O has completed
 * - Blocking that signal for all but one thread, whose job it is to
 *   receive that signal and mark each I/O in the bitmap complete
 *
 * ALTERNATIVE: Just use aio_suspend()
 *
 **/

/* signal that indicates we're done and the child thread can return to be
 * join()'ed with the parent thread */
#define MYSIG_STOP  SIGRTMIN

#include <stdio.h>
#include <signal.h>
#include <pthread.h>

int main(void)
{
  sigset_t         main_set, tid_set;
  struct sigaction act;
  union sigval     value;
  pthread_t        tid;

  /* Create a signal set for the child thread that will handle all Async I/O
   * completions */
  sigemptyset(&tid_set);
  sigaddset(&tid_set, SI_ASYNCIO);
  sigaddset(&tid_set, MYSIG_STOP);
  /* Create a signal set for the main thread which blocks the signals that the
   * child thread will handle */
  sigemptyset(&main_set);

}

static void *aio_complete_handler(void *arg)
{
  int       signum;
  siginfo_t info;

  do {
    signum = sigwaitinfo((sigset_t *)arg, &info);
    if (signum == SI_ASYNCIO)
      printf("ASYNC I/O COMPLETED\n");

    else if (signum == MYSIG_STOP) {
      printf("Got MYSIG_STOP; terminating thread\n");
      return (void *)true;
    }
    else
      printf("Got signal %d\n", signum);
    }
  } while (signum != -1 || errno == EINTR);
}

#include <stdio.h>
#include <mqueue.h>
#include <errno.h>

int
main()
{
  mqd_t  mq;

  /* mq_open calls __pos4obj_name() to create the filename, which is always
   * explicitly rooted at /tmp/ */
  if ((mq = mq_open("/mymq", O_RDWR|O_CREAT, 0644, NULL)) == (mqd_t)-1) {
    perror("Couldn't open mq");
  }
}

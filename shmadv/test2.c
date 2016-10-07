/* Here we just do a basic ISM shared memory segment */

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/shm_impl.h>
#include <sys/syscall.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

/* Pick a key for this test */
static int    SHKEY    = 0x600f;

/* This shared memory segment will be 63 MB */
#define SHM_SIZE  6ULL * 1024ULL * 1024ULL * 1024ULL

#define SVSHM_MODE (SHM_R | SHM_W | SHM_R >> 3 | SHM_R >> 6)

/* static unsigned char *local_buf[SHM_SIZE]; */
static unsigned char *mystruct[4];
/* static size_t         shm_size = 5ULL * 1024ULL * 1024ULL * 1024ULL; */
/* static unsigned long  struct_count = (shm_size / sizeof(mystruct)); */


int main(int argc, char **argv)
{
  int rc;
  int shmat_flag;
  int i, j,
      shmid;
  unsigned char *shmaddr    = NULL;
  uint_t         shm_advice;

  shm_advice = SHM_ACCESS_LWP;

  if ((shmid = shmget(SHKEY, SHM_SIZE, SVSHM_MODE | IPC_CREAT)) == -1) {
    perror("shmget failed");
    exit(1);
  }

  /* Set the memory allocation advice on the shared memory segment before it is
   * allocated */
  if (shmadv(shmid, SHM_ADV_SET, &shm_advice) == -1) {
    perror("shmadv failed");
    /* Destroy segment */
    shmctl(shmid,IPC_RMID,NULL);
    exit(3);
  }

  /* Attach as an ISM Shared Memory Segment */
  if ((shmaddr = shmat(shmid,NULL,SHM_SHARE_MMU)) == -1) {
    perror("shmat failed");
    /* Destroy segment */
    shmctl(shmid,IPC_RMID,NULL);
    exit(2);
  }

  /* for ISM, page allocation is not necessary, as the shmat above already
   * allocated all of the pages and locked them down */

  /* Quick and dirty page allocation test */
  /*
  for (j = 0; j < 100; j++) {
    for (i = 0; i < struct_count; i++) {
      memcpy(shmaddr   + (i * sizeof(struct_count)),
             local_buf + (i * sizeof(struct_count)),
             sizeof(struct_count));
    }
  }
  */

  /* Give time for system introspection */
  sleep(10);

  /*  Detach from segment */
  if (shmdt(shmaddr) == -1) {
    perror("SHM detach problem");
  }

  /* Destroy segment */
  shmctl(shmid,IPC_RMID,NULL);

  exit(0);
}

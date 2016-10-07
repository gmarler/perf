/* Here we just do a basic ISM shared memory segment */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

/* Pick a key for this test */
static int    SHKEY    = 0x600f;

/* This shared memory segment will be 63 MB */
#define SHM_SIZE  63 * 1024 * 1024

#define SVSHM_MODE (SHM_R | SHM_W | SHM_R >> 3 | SHM_R >> 6)

static unsigned char *local_buf[SHM_SIZE];
static unsigned char *mystruct[4];
static unsigned long  struct_count = (SHM_SIZE / sizeof(my_struct));


int main(int argc, char **argv)
{
  int rc;
  int shmat_flag;
  int i, j,
      shmid;
  unsigned char *shmaddr = NULL;

  if ((shmid = shmget(SHKEY, SHM_SIZE, SVSHM_MODE | IPC_CREAT)) == -1) {
    perror("shmget failed");
    exit(1);
  }

  /* Attach as an ISM Shared Memory Segment */
  shmaddr = shmat();
}

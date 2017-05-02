/* This test allows us to determine the optimimal copy time between two
 * ISM shared memory segments that are allocated in the same Locality Group, and
 * accessed by a thread in that same Locality Group.
 */

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
#include <time.h>

/* Pick a pair of keys for this test */
static int    SHKEY_SRC    = 0x6fff;
static int    SHKEY_DST    = 0x8111;

/* This shared memory segments will each be 64 MB */
#define SHM_SIZE  64ULL * 1024ULL * 1024ULL

#define SVSHM_MODE (SHM_R | SHM_W | SHM_R >> 3 | SHM_R >> 6)

/* We will break up the shared memory segments into 512 byte chunks  */
typedef struct {
  uchar_t data[512];
} memblock_t;

static unsigned long long struct_count = (SHM_SIZE / sizeof(memblock_t));


int main(int argc, char **argv)
{
  int rc;
  int shmat_flag;
  int i, j,
      shmid_src, shmid_dst;
  memblock_t *src    = NULL;
  memblock_t *dst    = NULL;
  memblock_t *src_iter    = NULL;
  memblock_t *dst_iter    = NULL;
  uint_t         shm_advice;
  int memsets[8][2] = {
    { 32,    71668 },
    { 64,    34432 },
    { 128,      25 },
    { 256,   64334 },
    { 512,  711446 },
    { 1024,  30707 },
    { 2048,    163 },
    { 4096,    163 },
  };
  int memcpys[10][2] = {
    { 1,      2109 },
    { 2,        73 },
    { 4,      5407 },
    { 8,       267 },
    { 32,    34677 },
    { 64,     6558 },
    { 128,   44834 },
    { 256,   34134 },
    { 512,  785408 },
    { 1024,     61 },
  };


  shm_advice = SHM_ACCESS_LWP;

  if ((shmid_src = shmget(SHKEY_SRC, SHM_SIZE, SVSHM_MODE | IPC_CREAT)) == -1) {
    perror("shmget for src failed");
    exit(1);
  }
  if ((shmid_dst = shmget(SHKEY_DST, SHM_SIZE, SVSHM_MODE | IPC_CREAT)) == -1) {
    perror("shmget for dst failed");
    exit(1);
  }

  /* Set the memory allocation advice on the shared memory segments before they
   * are allocated */
  if (shmadv(shmid_src, SHM_ADV_SET, &shm_advice) == -1) {
    perror("shmadv for src failed");
    /* Destroy segment */
    shmctl(shmid_src,IPC_RMID,NULL);
    exit(3);
  }
  if (shmadv(shmid_dst, SHM_ADV_SET, &shm_advice) == -1) {
    perror("shmadv for dst failed");
    /* Destroy segment */
    shmctl(shmid_dst,IPC_RMID,NULL);
    exit(3);
  }


  /* Attach each segment as an ISM Shared Memory Segment */
  if ((src = shmat(shmid_src,NULL,SHM_SHARE_MMU)) == -1) {
    perror("shmat for src failed");
    /* Destroy segment */
    shmctl(shmid_src,IPC_RMID,NULL);
    exit(2);
  }
  if ((dst = shmat(shmid_dst,NULL,SHM_SHARE_MMU)) == -1) {
    perror("shmat for dst failed");
    /* Destroy segment */
    shmctl(shmid_dst,IPC_RMID,NULL);
    exit(2);
  }


  /* for ISM, page allocation is not necessary, as the shmat above already
   * allocated all of the pages and locked them down */
  /* Test memset() first */
  /*  Size          Count
   *  32            71668
   *  64            34432
   *  128              25
   *  256           64334
   *  512          711446
   *  1024          30707
   *  2048            163
   *  4096            163
   *  */
  srand(time(NULL));
  long buffer_number;
  int row, size, count, toset, tocpy;
  for (row = 0; row < 8; row++) {
    size = memsets[row][0];
    count = memsets[row][1];
    printf("Setting memory of size %d %d times\n",size, count);
    for (toset = 0; toset < count; toset++) {
      buffer_number = ( rand() % struct_count );
      src_iter = &src[buffer_number];
      /* printf("Setting buffer %d with size %d\n",buffer_number, size); */
      memset(src_iter,'0',(size_t)size);
    }
  }
  /*
  for (src_iter = src, i = 0; i < struct_count; i++, src_iter++) {
    memset(src_iter,'0',sizeof(memblock_t));
  }
  */

  /* Then test memcpy() */
  for (row = 0; row < 10; row++) {
    size = memcpys[row][0];
    count = memcpys[row][1];
    printf("Copying memory of size %d %d times\n",size, count);
    for (tocpy = 0; tocpy < count; tocpy++) {
      buffer_number = ( rand() % struct_count );
      src_iter = &src[buffer_number];
      dst_iter = &dst[buffer_number];
      memcpy(dst_iter,src_iter,(size_t)size);
    }
  }

  /* Give time for system introspection */
  /*  sleep(10); */

  /*  Detach from segments */
  if (shmdt(src) == -1) {
    perror("SHM detach for src problem");
  }
  if (shmdt(dst) == -1) {
    perror("SHM detach for dst problem");
  }


  /* Destroy segments */
  shmctl(shmid_src,IPC_RMID,NULL);
  shmctl(shmid_dst,IPC_RMID,NULL);

  exit(0);
}

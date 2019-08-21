#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define SZ         32

int main(int argc, char **argv)
{
  int i, n = 0;
  char *pp[SZ+1];

  for (n = 0; n < SZ; n++) {
    pp[n] = malloc(1<<20);  /* 1 MB */
    if (pp[n] == NULL) {
      perror("UNEXPECTED malloc failure");
      printf("ERRNO: %d\n", errno);
      printf("malloc failure after %d MB\n", n);
      exit(1);
    }
  }
  pp[SZ] = malloc(1);
  if (pp[SZ] == NULL) {
    perror("EXPECTED malloc failure");
    exit(0);
  }

  for (i = 0; i < n; i++) {
    usleep(250000);
    memset(pp[i], 1, (1<<20));
    printf("%d MB\n", i+1);
  }
  memset(pp[SZ], 1, 1);
  sleep(20);
}

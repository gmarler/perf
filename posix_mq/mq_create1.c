#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <mqueue.h>

#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define PRIV_MODE (S_IRUSR | S_IWUSR)

int
main(int argc, char **argv)
{
  int    c, flags;
  mqd_t  mqd;

  flags = O_RDWR | O_CREAT;
  while (( c = getopt(argc, argv, "e")) != -1) {
    switch (c) {
      case 'e' :
        flags |= O_EXCL;
        break;
    }
  }

  if (optind != argc - 1) {
    printf("usage: mqcreate [ -e ] <name>\n");
    exit(1);
  }

  mqd = mq_open(argv[optind], flags, PRIV_MODE, NULL);

  mq_close(mqd);
  exit(0);
}

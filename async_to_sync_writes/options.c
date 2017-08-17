/******************************************************************************
 * Options for this test:
 * --filepath=/path/to/file
 *  Path to the file you want this test to write out
 * --filesize=<size in bytes>
 *  Total size of the file this test will create
 * --blocksize=<size in bytes>
 *  Size of each write to the file
 * --test=(pwrite|aio_write|lio_listio)
 *  The way to perform writes - synchronous or asynchronous
 * --sync_type=(O_SYNC|O_DSYNC|<none - async - default)
 *  Whether I/O is synchronized or non-synchronized (default)
 ******************************************************************************/


#include "test_type.h"
#include "options.h"

int
collect_options(int *argc, char **argv, char *filepath,
                long long *filesize,
                long long *blocksize,
                enum test_type *test, int *sync_type)
{
  char                 *eptr;
  int                   c;
  static struct option  long_options[] =
  {
    {"filepath",  required_argument, 0, 'f'},
    {"filesize",  required_argument, 0, 's'},
    {"blocksize", required_argument, 0, 'b'},
    {"test",      required_argument, 0, 't'},
    {"sync_type", required_argument, 0, 'y'},
    {0,                           0, 0,   0}
  };

  while (1)
  {
    int option_index = 0;
    c = getopt_long(*argc, argv, "f:s:b:t:y:",
                    long_options, &option_index);

    /*  Detect the end of the options. */
    if (c == -1)
      break;

    switch (c)
    {
      case 'f':
        printf("FilePath: %s\n", optarg);
        strcpy(filepath, optarg);
        break;
      case 's':
        *filesize = strtoll(optarg, &eptr, 10);
        printf("filesize: %lld\n", *filesize);
        break;
      case 'b':
        *blocksize = strtoll(optarg, &eptr, 10);
        printf("blocksize: %lld\n", *blocksize);
        break;
      case 't':
        printf("test: %s\n", optarg);
        if (strcmp(optarg,"pwrite") == 0) {
          *test = Test_pwrite;
        } else if (strcmp(optarg,"aio_write") == 0) {
          *test = Test_aio_write;
        } else if (strcmp(optarg,"lio_listio") == 0) {
          *test = Test_lio_listio;
        }
        break;
      case 'y':
        printf("sync_type: %s\n", optarg);
        if (strcmp(optarg,"O_SYNC") == 0) {
          *sync_type = O_SYNC;
        } else if (strcmp(optarg,"O_DSYNC") == 0) {
          *sync_type = O_DSYNC;
        } else {
          *sync_type = 0;
        }
        break;
      default:
        abort();
    }
  }

}

void usage(char **argv)
{
  printf("Usage: %s\n",argv[0]);
  printf("--filepath=</path/to/file>\n");
  printf("  Path to file to write\n");
  printf("--filesize=<size in bytes>\n");
  printf("  Size of the file you want to create\n");
  printf("--blocksize=<size in bytes>\n");
  printf("  Each I/O to the file will be of this size\n");
  printf("--test=(pwrite|aio_write|lio_listio)\n");
  printf("  synchronous or asynchronous write type\n");
  printf("--sync_type=(O_SYNC|O_DSYNC|<none - default>)\n");
  printf("  synchronized or non-synchronized I/Os\n");

  exit(0);
}

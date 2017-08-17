#include <stdio.h>
#include <limits.h>
#include "test_type.h"
#include "options.h"

int main(int argc, char **argv)
{
  char            filepath[PATH_MAX];
  long long       filesize;
  long long       blocksize;
  enum test_type  test;
  int             sync_type;

  collect_options(&argc, argv, filepath, &filesize, &blocksize, &test, &sync_type);

}

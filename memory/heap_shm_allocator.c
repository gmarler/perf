/*
 * Purpose: Allocate any arbitrary amount of heap of shared memory, to test
 * whether Private RSS memory statistics for any given process are actually
 * correct. */

#include <pcre/pcre.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <strings.h>

/* For Linux, see if /proc/<PID>/status file contains this line, or abort */

int private_rss_supported(void) {
  pid_t mypid;
  struct stat stat_buffer;
  int status_exists, private_rss_exists;
  char private_rss_filename[1024];
  FILE *fp;
  char *line = NULL;
  ssize_t read_bytes;
  size_t  len = 0;

  mypid = getpid();
  sprintf(private_rss_filename, "/proc/%d/status", mypid);
  status_exists = stat(private_rss_filename, &stat_buffer);
  if (status_exists == 0) {
    /* The file exists, check for the proper line in it */
    fp = fopen(private_rss_filename, "r");
    if (fp == NULL) {
      printf("Failed to open %s\n", private_rss_filename);
      exit(1);
    }

    while ((read_bytes = getline(&line, &len, fp)) != -1) {
      if (strstr(line, "RssAnon:")) {
        /* If we found ^RssAnon:, we're good here */
        return 1;
      }
    }
    /* If we got here, we didn't find the line we need, and can
     * return false*/
    return 0;

  } else {
    printf("The file %s does not exist, EXITING\n",private_rss_filename);
    return 0;
  }
}

int main() {
  if (private_rss_supported()) {
    printf("We can check Private RSS per process.\n");
  }
}

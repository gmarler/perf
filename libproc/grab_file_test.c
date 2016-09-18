#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <procfs.h>
#include <sys/procfs.h>
#include "libproc.h"

int proc_object_iter(void *, const prmap_t *, const char *);
int file_object_iter(void *, const prmap_t *, const char *);

int main(int argc, char **argv)
{
  pid_t                 pid_of_interest;
  int                   perr;
  struct ps_prochandle *pshandle;

  if (argc == 2) {
    pid_of_interest = atoi(argv[1]);
    printf("You want to examine PID: %u\n",pid_of_interest);
  } else {
    printf("Must specify PID\n");
    exit(1);
  }

  /* Use PGRAB_RDONLY to avoid perturbing the target PID */
  pshandle = Pgrab(pid_of_interest, PGRAB_RDONLY, &perr);

  if (pshandle == NULL) {
    printf("Unable to attach: %s\n",Pgrab_error(perr));
    exit(2);
  }

  /* NOTE: Passing pshandle in as cd argument for use by Psymbol_iter later */
  Pobject_iter(pshandle, proc_object_iter, (void *)pshandle);

  Pfree(pshandle);
}

int
proc_object_iter(void *Proc, const prmap_t *pmp, const char *object_name)
{
  struct ps_prochandle *file_pshandle;
  int                   perr;

  printf("%-120s\n", object_name);
  /* For each object name, grab the file, then iterate over the objects,
   * extracting the symbol table */
  if ((file_pshandle = Pgrab_file(object_name, &perr)) == NULL) {
    printf("Unable to grab file: %s\n",Pgrab_error(perr));
  }
  return 0;
}

int
file_object_iter(void *Proc, const prmap_t *pmp, const char *object_name)
{
  printf("%-120s\n", object_name);
  return 0;
}


#include <stdio.h>
#include <stdlib.h>

#include <procfs.h>
#include <sys/procfs.h>
#include "libproc.h"

int object_iter(void *, const prmap_t *, const char *);

int main(int argc, char **argv)
{
  pid_t pid_of_interest;
  int   perr;
  void *pshandle;

  if (argc == 2) {
    pid_of_interest = atoi(argv[1]);
    printf("You want to examine PID: %u\n",pid_of_interest);
  } else {
    printf("Must specify PID\n");
    exit(1);
  }

  /* Should we use PGRAB_RDONLY? */
  pshandle = Pgrab(pid_of_interest, PGRAB_RDONLY, &perr);

  if (pshandle == NULL) {
    printf("Unable to attach: %s\n",Pgrab_error(perr));
    exit(2);
  }

  printf("%-120s %16s\n","OBJECT","BASE ADDRESS");
  Pobject_iter(pshandle, object_iter, NULL);

  Pfree(pshandle);
}

int
object_iter(void *cd, const prmap_t *pmp, const char *object_name)
{
  printf("%-120s %016llX\n",object_name, pmp->pr_vaddr);
  /*  Psymbol_iter(cd, object_name, , mask,
   *               PRO_NATURAL, (proc_sym_f *)function_iter, DERP);
   *  */
  return 0;
}

int
function_iter()
{
  return 0;
}

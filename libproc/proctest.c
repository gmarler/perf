#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <procfs.h>
#include <sys/procfs.h>
#include "libproc.h"

int object_iter(void *, const prmap_t *, const char *);
int function_iter(void *arg, const GElf_Sym *sym, const char *func_name);

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

  printf("%-120s %16s\n","OBJECT","BASE ADDRESS");
  /* NOTE: Passing pshandle in as cd argument for use by Psymbol_iter later */
  Pobject_iter(pshandle, object_iter, (void *)pshandle);

  Pfree(pshandle);
}

int
object_iter(void *Proc, const prmap_t *pmp, const char *object_name)
{
  long function_count = 0;

  printf("%-120s %016llX\n", object_name, pmp->pr_vaddr);
  Psymbol_iter((struct ps_prochandle *)Proc,
               object_name,
               PR_SYMTAB,
               BIND_GLOBAL | TYPE_FUNC,
               function_iter,
               (void *)&function_count);

  printf("FUNCTION COUNT: %ld\n", function_count);
  return 0;
}

int
function_iter(void *function_count, const GElf_Sym *sym, const char *sym_name)
{
  if (sym_name != NULL) {
    printf("\t%32s %llu %llu\n", sym_name, sym->st_value, sym->st_size);
    (*((long *)function_count))++;
  } else {
    printf("\tNULL FUNCNAME\n");
  }
  return 0;
}

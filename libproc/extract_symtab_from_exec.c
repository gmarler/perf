#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <procfs.h>
#include <sys/procfs.h>
#include "libproc.h"

/* used to pass information between Pobject_iter()'s caller and callback */
typedef struct {
  struct ps_prochandle  *file_pshandle;
  long                   function_count;
  /* TODO: Add something to show what the type of the file is, so we know how to
   * handle symbol resolution properly - static a.out or dynamic library */
} data_t;

int proc_object_iter(void *, const prmap_t *, const char *);
int function_iter(void *arg, const GElf_Sym *sym, const char *func_name);

int main(int argc, char **argv)
{
  const char           *a_out;
  int                   perr;
  struct ps_prochandle *exec_handle;

  if (argc == 2) {
    a_out = argv[1];
    /* printf("You want to examine executable: %s\n",a_out); */
  } else {
    printf("Must specify an executable\n");
    exit(1);
  }

  /* Use PGRAB_RDONLY to avoid perturbing the target PID */
  if ((exec_handle = Pgrab_file(a_out, &perr)) == NULL) {
    printf("Unable to grab file: %s\n",Pgrab_error(perr));
    exit(2);
  }

  /* NOTE: Passing pshandle in as cd argument for use as first argument of
   * Psymbol_iter later */
  Pobject_iter(exec_handle, proc_object_iter, (void *)NULL);

  Pfree(exec_handle);
}

int
proc_object_iter(void *callback_arg, const prmap_t *pmp, const char *object_name)
{
  data_t                procfile_data;
  struct ps_prochandle *file_pshandle;
  int                   perr;


  /* printf("proc_object_iter: %-120s\n", object_name); */
  /* For each object name, grab the file, then iterate over the objects,
   * extracting their symbol tables */
  if ((file_pshandle = Pgrab_file(object_name, &perr)) == NULL) {
    printf("Unable to grab file: %s\n",Pgrab_error(perr));
  }
  /* NOTE: Passing file_pshandle in for use as callback argument for
   * Psymbol_iter later */
  procfile_data.file_pshandle = file_pshandle;
  procfile_data.function_count = 0;

  Psymbol_iter(file_pshandle,
               object_name,
               PR_SYMTAB,
               BIND_GLOBAL | TYPE_FUNC,
               function_iter,
               (void *)&procfile_data);

  /* printf("FUNCTION COUNT: %ld\n", procfile_data.function_count); */

  return 0;
}



int
function_iter(void *callback_arg, const GElf_Sym *sym, const char *sym_name)
{
  data_t procfile_data = (*((data_t *)callback_arg));

  if (sym_name != NULL) {
    printf("\t%32s %llu %llu\n", sym_name, sym->st_value, sym->st_size);
    (((data_t *)callback_arg)->function_count)++;
  } else {
    printf("\tNULL FUNCNAME\n");
  }
  return 0;
}


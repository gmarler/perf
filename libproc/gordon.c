#include <libgen.h>
#include <procfs.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/int_types.h>


/* libproc interface definitions */
typedef struct ps_prochandle ps_prochandle_t;
typedef int proc_map_f(void *, const prmap_t *, const char *);
 
void Pfree(ps_prochandle_t *);
const char *Pgrab_error(int);
int Pobject_iter(ps_prochandle_t *, proc_map_f, void *);
ps_prochandle_t *proc_arg_grab(const char *, int, int, int *, const char **);

#define	PR_ARG_PIDS	0x1	/* Allow pid and /proc file arguments */
#define	PGRAB_RDONLY	0x04	/* Open the process or core w/ O_RDONLY */


/* used to pass information between Pobject_iter()'s caller and callback */
typedef struct {
	const char	*search_name;
	uintptr_t	search_addr;
} search_t;

/*
 * This will be called once for each object in the process's link map.
 *
 * By returning non-zero, the function indicates to its caller that iteration
 * should cease.
 */
int
callback(void *arg, const prmap_t *prmap, const char *name)
{
	search_t *sp = (search_t *)arg;
	char *copy;
	char *base;
	boolean_t match;

	if (name == NULL)
		return (0);

	if ((copy = strdup(name)) == NULL)
		return (0);

	base = basename(copy);

	match = (strcmp(sp->search_name, base) == 0) ? B_TRUE : B_FALSE;

	free(copy);

	if (match == B_FALSE)
		return (0);

	/*
	 * Record the base address of the primary text mapping, i.e. the
	 * object's load address.
	 */
	sp->search_addr = prmap->pr_vaddr;

	return (1);
}

int
main(int argc, char **argv)
{
	int perr;
	ps_prochandle_t *Pr;
	search_t s;
	int result;

	if (argc != 3) {
		fprintf(stderr, "usage: %s <PID> <object>\n", argv[0]);
		return (1);
	}

	/* Grab the process. */
	Pr = proc_arg_grab(
	    argv[1],		/* argument */
	    PR_ARG_PIDS,	/* interpret argument as PID */
	    PGRAB_RDONLY,	/* don't interfere with process */
	    &perr,		/* any error */
	    NULL		/* do not specify LWP(s) */
	    );

	if (Pr == NULL) {
		fprintf(stderr, "proc_arg_grab() failed with %s\n",
		    Pgrab_error(perr));
		return (1);
	}
	
	/* Walk the object in the process's link map. */
	s.search_name = argv[2];
	result = Pobject_iter(
	    Pr,			/* handle */
	    callback,		/* call once for each object */
	    &s			/* argument to callback function */
	    );

	if (result != 0) {
		fprintf(stdout, "%s loaded at 0x%llx\n",
		    s.search_name, s.search_addr);
	} else {
		fprintf(stdout, "%s not found\n", s.search_name);
	}

	/* Free the libproc resources and leave the process untouched. */
	Pfree(Pr);

	return (0);
}


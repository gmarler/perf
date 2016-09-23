#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <libelf.h>

int
main(int argc, char *argv[])
{
  const char *file = argv[1];
  int         fd;
  Elf        *elf;

  printf("Examining ELF file: %s\n",file);

  if ((fd = open(file, O_RDONLY)) == -1) {
    int err = errno;
    (void) fprintf(stderr, "Unable to open %s\n",
                   file, strerror(err));
  }

  (void) elf_version(EV_CURRENT);
  if ((elf = elf_begin(fd, ELF_C_READ, NULL)) == NULL) {
    printf("Failed to open ELF file %s\n",file);
    (void) close(fd);
    exit (0);
  }

  switch (elf_kind(elf)) {
    case ELF_K_ELF:
/*    ret     = decide(file, fd, elf, flags, wname, wfd, osabi); */
      printf("This is an ELF file\n");
      break;
    default:
      (void) fprintf(stderr, "%s: DOES NOT LOOK LIKE AN ELF FILE\n", file);
      break;
  }

  (void) close(fd);
  (void) elf_end(elf);
}

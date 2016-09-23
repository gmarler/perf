#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <libelf.h>
#include <gelf.h>
#include <sys/machelf.h>

#include "libelftest.h"

int
main(int argc, char *argv[])
{
  const char *file = argv[1];
  int         fd,
              ret;
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
      ret     = determine_bitness(file, fd, elf);
      printf("This is an ELF file\n");
      break;
    default:
      (void) fprintf(stderr, "%s: DOES NOT LOOK LIKE AN ELF FILE\n", file);
      break;
  }

  (void) close(fd);
  (void) elf_end(elf);
}

int
determine_bitness(const char *file, int fd, Elf *elf)
{
  int r = 0;

  if (gelf_getclass(elf) == ELFCLASS64) {
    printf("ELFCLASS64\n");
    r = elfheader64bit(file, fd, elf);
  }
  else {
    printf("ELFCLASS32\n");
    r = elfheader32bit(file, fd, elf);
    /* r = elfheader64bit(file, fd, elf); */
  }

  return (r);
}

int
elfheader64bit(const char *file, int fd, Elf *elf)
{
  int          ret = 0;
  Elf64_Ehdr  *ehdr;
  Elf64_Shdr  *shdr;
  Elf_Scn     *scn;

  if ((ehdr = elf64_getehdr(elf)) == NULL) {
    printf("Failed to extract EHDR from %s\n",file);
    return (ret);
  }

  if ((scn     = elf_getscn(elf, 0)) != NULL) {
    if ((shdr = elf64_getshdr(scn)) == NULL) {
      (void) fprintf(stderr, "Unable to extract ELF section headers\n", 0);
      return (ret);
    }
  } else
    shdr      = NULL;

  Elf_ehdr(0, ehdr, shdr);
}

int
elfheader32bit(const char *file, int fd, Elf *elf)
{
  int          ret = 0;
  Elf32_Ehdr  *ehdr;

  if ((ehdr = elf32_getehdr(elf)) == NULL) {
    printf("Failed to extract EHDR from %s\n",file);
    return (ret);
  }

}

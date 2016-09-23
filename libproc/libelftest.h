#ifndef __LIBELFTEST_H__
#define __LIBELFTEST_H__

int determine_bitness(const char *file, int fd, Elf *elf);
int elfheader32bit(const char *file, int fd, Elf *elf);
int elfheader64bit(const char *file, int fd, Elf *elf);

#endif /* __LIBELFTEST_H__ */

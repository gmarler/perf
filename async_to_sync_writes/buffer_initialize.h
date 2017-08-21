#ifndef BUFFER_INITIALIZE_H
#define BUFFER_INITIALIZE_H
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>


void buffer_initialize(char **buffers,int buffer_count, int bufsize);

#endif /* BUFFER_INITIALIZE_H */

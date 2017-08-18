#ifndef BUFFER_INITIALIZE_H
#define BUFFER_INITIALIZE_H
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>


void buffer_initialize(char *buffers,int buffers, int bufsize);

#endif /* BUFFER_INITIALIZE_H */

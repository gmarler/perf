#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>

int main(int argc, char **argv)
{
  struct hostent *myhost;
  int             error_num;

  if (argc != 2) {
    printf("usage: %s hostname\n", argv[0]);
    exit(1);
  }

  myhost = getipnodebyname( argv[1], AF_INET, AI_DEFAULT, &error_num);

  if (myhost == NULL) {
    printf("%s: Unknown hostname\n", argv[1]);
    exit(1);
  }

  exit(0);
}


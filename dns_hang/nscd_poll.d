#!/usr/sbin/dtrace -s

syscall::pollsys:entry
/(execname == "nscd") &&
 arg0 && (arg1 == 1)
/
{
  self->in_pollsys = 1;
  self->pfds_user = arg0;
  self->fds_cnt = (int)arg1;
  this->pfds = (struct pollfd*) copyin(self->pfds_user, sizeof(struct pollfd));

  printf("ENTRY pid: %d Pollsys thread = %d npfds = %d\n",
          pid, tid, arg1);

  printf("%s:%s fd = %d, events = %x, revents = %x\n",
          probefunc, probename,
          this->pfds->fd, this->pfds->events, this->pfds->revents);
}

/* Only if fd is sockfs
 * Only if fd is UDP socket
 * Only if fd remote host is one of the BDNS servers
 */

#!/usr/sbin/dtrace -s

#pragma D option flowindent

pid$target::gethost*:entry
{
  self->trace = 1;
  @[probeprov,probemod,probefunc,probename] = count();
}

pid$target::*nss*:entry,
pid$target::*res*:entry,
pid$target::*nss*:return,
pid$target::*res*:return
/ self->trace /
{
}

pid$target::gethost*:return
/ self->trace /
{
  self->trace = 0;
}

END {
  printa(@);
}

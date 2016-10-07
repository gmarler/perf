This evaluation has to do with ISM shared memory segments in Solaris spanning
more than one Locality Group when you wish them to only exist in the single
Locality Group where a single threaded process resides.

Looks like you need to use the **shmadv(2)** system call for this, so that's
what we're trying.

The methodology for testing is to force the test process (test2 at the moment)
to run in a particular Locality group via **plgrp(1)**, like so, in which we
force the process to have a "Home Locality Group" of 4 on a T5-4:

```
plgrp -H 4 -e ./test2
```

In a separate window, use **pmap(1)** with the -L option in the allowed 10
second window that test2 runs in to confirm that the memory allocation of the
ISM shared memory segment occurs in the expected Locality Group:

```
# pmap -L $(pgrep test2)
5872:   ./test2
0000000100000000          8K r-x--    2 /bb/pm/bin/test2
0000000100100000          8K rwx--    2 /bb/pm/bin/test2
0007FFFD00000000    6291456K rwxsR    2   [ ism shmid=0x2000050 ]
0007FFFEFF600000         16K rwx--    2   [ anon ]
0007FFFEFF604000          8K rwx--    -   [ anon ]
0007FFFEFF700000         16K rw---    2   [ anon ]
0007FFFEFF800000        640K r-x--    1 /lib/sparcv9/libc.so.1
0007FFFEFF8A0000         64K r-x--    - /lib/sparcv9/libc.so.1
0007FFFEFF8B0000        256K r-x--    1 /lib/sparcv9/libc.so.1
0007FFFEFF8F0000        128K r-x--    - /lib/sparcv9/libc.so.1
0007FFFEFF910000         64K r-x--    1 /lib/sparcv9/libc.so.1
0007FFFEFF920000        320K r-x--    - /lib/sparcv9/libc.so.1
0007FFFEFF970000         64K r-x--    1 /lib/sparcv9/libc.so.1
0007FFFEFFA80000         64K rwx--    2 /lib/sparcv9/libc.so.1
0007FFFEFFA90000          8K rwx--    2 /lib/sparcv9/libc.so.1
0007FFFEFFB00000          8K rw---    2   [ anon ]
0007FFFEFFC00000          8K rw---    2   [ anon ]
0007FFFEFFD00000          8K rw---    2   [ anon ]
0007FFFEFFE00000        256K r-x--    1 /lib/sparcv9/ld.so.1
0007FFFEFFE40000          8K r-x--    1 /lib/sparcv9/ld.so.1
0007FFFEFFE42000          8K r-x--    2 /lib/sparcv9/ld.so.1
0007FFFEFFF44000         24K rwx--    2 /lib/sparcv9/ld.so.1
FFFFFFFF7FFF0000         64K rw---    2   [ stack ]
         total      6293504K
```

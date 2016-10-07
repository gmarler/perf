This evaluation has to do with ISM shared memory segments in Solaris spanning
more than one Locality Group when you wish them to only exist in the single
Locality Group where a single threaded process resides.

Looks like you need to use the **shmadv(2)** system call for this, so that's
what we're trying.

The methodology for testing is to force the test process (test2 at the moment)
to run in a particular Locality group via **plgrp(1)**, like so, in which we
force the process to have a "Home Locality Group" of 4 on a T5-4:

```
plgrp -H 3 -e ./test2
```

In a separate window, use **pmap(1)** with the -L option in the allowed 10
second window that test2 runs in to confirm that the memory allocation of the
ISM shared memory segment occurs in the expected Locality Group:

```
# pmap -Ls $(pgrep test2)
45897:  ./test2
         Address       Bytes Pgsz Mode    Lgrp Mapped File
0000000100000000          8K   8K r-x----    3 /bb/pm/bin/test2
0000000100100000          8K   8K rwx----    3 /bb/pm/bin/test2
0007FFFD00000000    6291456K 256M rwxsR--    3   [ ism shmid=0x39000e88 ]
FFFFFFFF34000000        256K  64K r-x----    1 /lib/sparcv9/libc.so.1
FFFFFFFF34040000         64K  64K r-x----    2 /lib/sparcv9/libc.so.1
FFFFFFFF34050000         64K  64K r-x----    1 /lib/sparcv9/libc.so.1
FFFFFFFF34060000        256K    - r-x----    - /lib/sparcv9/libc.so.1
FFFFFFFF340A0000        384K  64K r-x----    1 /lib/sparcv9/libc.so.1
FFFFFFFF34100000        128K    - r-x----    - /lib/sparcv9/libc.so.1
FFFFFFFF34120000        192K  64K r-x----    1 /lib/sparcv9/libc.so.1
FFFFFFFF34150000        192K    - r-x----    - /lib/sparcv9/libc.so.1
FFFFFFFF34180000          8K    - r-x----    - /lib/sparcv9/libc.so.1
FFFFFFFF34182000         16K   8K r-x----    1 /lib/sparcv9/libc.so.1
FFFFFFFF34186000         16K    - r-x----    - /lib/sparcv9/libc.so.1
FFFFFFFF3428A000          8K    - rwx----    - /lib/sparcv9/libc.so.1
FFFFFFFF3428C000         40K   8K rwx----    3 /lib/sparcv9/libc.so.1
FFFFFFFF34296000          8K    - rwx----    - /lib/sparcv9/libc.so.1
FFFFFFFF34298000          8K   8K rwx----    3 /lib/sparcv9/libc.so.1
FFFFFFFF3429A000          8K   8K rwx----    3 /lib/sparcv9/libc.so.1
FFFFFFFF7F100000         16K   8K rwx----    3   [ anon ]
FFFFFFFF7F104000          8K    - rwx----    -   [ anon ]
FFFFFFFF7F200000         64K  64K rw-----    3   [ anon ]
FFFFFFFF7F306000          8K    - r--s---    -   [ anon ]
FFFFFFFF7F400000        256K  64K r-x----    1 /lib/sparcv9/ld.so.1
FFFFFFFF7F440000          8K   8K r-x----    4 /lib/sparcv9/ld.so.1
FFFFFFFF7F442000          8K   8K r-x----    2 /lib/sparcv9/ld.so.1
FFFFFFFF7F544000         24K   8K rwx----    3 /lib/sparcv9/ld.so.1
FFFFFFFF7F604000          8K    - r--s---    -   [ anon ]
FFFFFFFF7F702000          8K    - r--s---    -   [ anon ]
FFFFFFFF7FFF0000         64K  64K rw-----    3   [ stack ]
         total      6293592K
```

Note that the segment was fully allocated in Locality Group 3, and constructed
of 256 MB pages.

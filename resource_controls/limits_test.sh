#!/bin/ksh

# ulimit -f 63 (32KB = 63 * 512 bytes)
# Launch the process which writes some random file
./file_writer &
FILE_CHILD=$!
print -u2 "Launched file_writer child PID ${FILE_CHILD}"
# Set the process.max-file-size to 32768 (which is what "ulimit -f 63" does under the covers anyway)
prctl -n process.max-file-size -v 32768 -r -i pid ${FILE_CHILD}
prctl -n process.max-file-size ${FILE_CHILD}
wait ${FILE_CHILD}


# ulimit -d
./memory_alloc &
MEMORY_CHILD=$!
print -u2 "Launched memory_alloc child PID ${MEMORY_CHILD}"
# Set the process.max-data-size to 32MB (which is what "ulimit -d 32768" does under the covers anyway)
prctl -n process.max-data-size -v 33554432 -r -i pid ${MEMORY_CHILD}
prctl -n process.max-data-size ${MEMORY_CHILD}
wait ${MEMORY_CHILD}

print -u2 "EXITING"


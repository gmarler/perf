#ifndef MEMORYFENCE_H
#define MEMORYFENCE_H

/*
 * This file contains functions to allow safe ordered lock-free memory access
 * on weakly ordered memory systems.
 *
 * In weakly ordered memory, the order of writes to memory is not guaranteed
 * to be the same as the order of writes in the code. This is particularly
 * important for applications where there are multiple threads or where shared
 * memory is involved.
 *
 * For example, assume thread A and thread B are running on different CPUs.
 *
 * Currently new_value contains 0 and new_value_updated contains false; both
 * are declared to be volatile.
 *
 * A runs this code:
 *     new_value = 1;
 *     new_value_updated = true;
 *
 * B runs this code:
 *     while (!new_value_updated) do {};
 *     cout << "Value is " << new_value << endl;
 *
 * B can output 0 or 1, as the real memory update for new_value_updated can
 * happen before that of new_value (even WITHOUT optimisation).
 *
 * In order to control this behaviour, and you are in a position where locks
 * are not appropriate (obtaining a lock or using conditional mutexes are,
 * in general, more suitable and perform any memory synchronisation required),
 * the following mechanisms should be used:
 *
 * WRITER:
 *  new_value = 1;
 *  MF_write_sync();
 *  new_value_updated  = true;
 *
 * READER:
 *  while (!new_value_updated) {};
 *  MF_read_sync();
 *  cout << "Value is " << new_value << endl;
 *
 * MF_full_sync combines both reader and writer semantics, and is provided
 * for completeness only.
 */

#include "membar.h"

#define MF_read_sync()  MEMBAR_acquire()
#define MF_write_sync() MEMBAR_release()
#define MF_full_sync()  MEMBAR_fullsync()

#endif /* MEMORYFENCE_H */

/* 
 * Similar to aio_signal_test, but uses Solaris Port events instead of signals
 * to indicate that each I/O has completed:
 *
 * For this test, we will be doing the following:
 * - Opening a file "synchronized" (O_DSYNC)
 * - But performing "asynchronous" writes to it (aio_write)
 * - Creating a bitmap for all of the outgoing writes, initialized to all zeros
 * - Using port events in a single thread to inform us that each I/O has completed
 *
 * ALTERNATIVE: Just use aio_suspend() 
 *
 *
 * */

/*
 * For this test, we will be doing the following:
 * - Opening a file "synchronized" (O_DSYNC)
 * - But performing "asynchronous" writes to it (aio_write)
 * - Creating a bitmap for all of the outgoing writes, initialized to all zeros
 * - Picking a signal to inform us that each I/O has completed
 * - Blocking that signal for all but one thread, whose job it is to
 *   receive that signal and mark each I/O in the bitmap complete
 *
 * ALTERNATIVE: Just use aio_suspend() 
 *
 **/



/*
 * membar.h
 *
 *   macros to implement portable memory barriers
 *   (for use in lockless algorithms in reentrant, thread-safe code)
 *
 * See http://en.cppreference.com/w/c/atomic/memory_order for technical overview
 * of memory ordering.  The memory barriers below aim to provide equivalent or
 * stronger guarantees than those provided by C11/C++11 atomic_thread_fence().
 *
 */


#ifndef INCLUDED_MEMBAR_H
#define INCLUDED_MEMBAR_H

#include <stdint.h>

#if defined(__sparc) || defined(__sparc__)

  #include <atomic.h>

  #define MEMBAR_ccfence() \
          asm volatile ("" : : : "memory")

  /* NOTE: ACQUIRE and RELEASE barriers here are stronger than needed.
   * SPARC runs in TSO (total store order) and only StoreLoad barrier must
   * be explicitly specified when needed.  (There is some older, buggy SPARC
   * chips that need stronger barriers, but those are ancient we need not
   * support them here.)  Therefore, acquire and release need only be compiler
   * fences.  To ensure compatibility with previous code that might have misused
   * acquire or release when it really needed StoreLoad, this code is not being
   * changed.  Instead, new macros are introduced (with lowercased suffix). */

  #define MEMBAR_StoreStore() \
          asm volatile ("membar #StoreStore" : : : "memory")
  #define MEMBAR_storestore() \
          asm volatile ("" : : : "memory")

  #define MEMBAR_ACQUIRE() \
          asm volatile ("membar #StoreLoad|#StoreStore" : : : "memory")
  #define MEMBAR_acquire() \
          asm volatile ("" : : : "memory")

  #define MEMBAR_RELEASE() \
          asm volatile ("membar #LoadStore|#StoreStore" : : : "memory")
  #define MEMBAR_release() \
          asm volatile ("" : : : "memory")

  #define MEMBAR_FULLSYNC() \
          asm volatile ("membar #Sync" : : : "memory")
          /*(might instead use #MemIssue instead of full #Sync)*/

  #define MEMBAR_LOCK_CAS(ptr) \
          atomic_cas_32((ptr), 0, 1)
          /*(inline assembly would require args in registers)*/
          /*(Sun Studio requires optimization for proper inline assembly cas)*/

  #define MEMBAR_LOCK_RELEASE(ptr) \
          MEMBAR_RELEASE(); *(ptr) = 0

#elif defined(__ppc__)   || defined(_ARCH_PPC)  || \
      defined(_ARCH_PWR) || defined(_ARCH_PWR2) || defined(_POWER)

  /* POWER chips */

 #ifdef _AIX
  #include <sys/atomic_op.h>
 #endif

 #if !defined(__GNUC__)

  #if defined(__xlc__) || defined(__xlC__)
  #ifdef __cplusplus
  #include <builtins.h>
  #endif
  #endif

  #define MEMBAR_ccfence() \
          __fence()

  #define MEMBAR_StoreStore() \
          __lwsync()

  #define MEMBAR_ACQUIRE() \
          __isync()
  #define MEMBAR_acquire() \
          __lwsync()
          /* __lwsync() faster than __isync() on POWER6 and above
           * and same/negligible performance difference on POWER5 */

  #define MEMBAR_RELEASE() \
          __lwsync()

  #define MEMBAR_FULLSYNC() \
          __sync()

  #define MEMBAR_LOCK_CAS(ptr) \
          __check_lock_mp((atomic_p)(ptr), 0, 1)

  #define MEMBAR_LOCK_RELEASE(ptr) \
          __clear_lock_mp((atomic_p)(ptr), 0)

 #else /* __GNUC__ */

  #define MEMBAR_ccfence() \
          asm __volatile__ ("" : : : "memory")

  #define MEMBAR_StoreStore() \
          asm __volatile__ ("lwsync" : : : "memory")

  #define MEMBAR_ACQUIRE() \
          asm __volatile__ ("isync" : : : "memory")
  #define MEMBAR_acquire() \
          asm __volatile__ ("lwsync" : : : "memory")

  #define MEMBAR_RELEASE() \
          asm __volatile__ ("lwsync" : : : "memory")

  #define MEMBAR_FULLSYNC() \
          asm __volatile__ ("sync" : : : "memory")

 #ifdef _AIX

  #define MEMBAR_LOCK_CAS(ptr) \
          _check_lock((atomic_p)(ptr), 0, 1)

  #define MEMBAR_LOCK_RELEASE(ptr) \
          _clear_lock((atomic_p)(ptr), 0)

 #elif defined(__linux__)

  #define MEMBAR_LOCK_CAS(ptr) \
          (!!__sync_val_compare_and_swap((ptr), 0, 1))
          /* force bool context */

  #define MEMBAR_LOCK_RELEASE(ptr) \
          __sync_lock_release(ptr)

 #endif

 #endif /* __GNUC__ */

#elif defined(__i386__) || defined(__x86_64__)
  /* x86{,_64} chips */

  #define MEMBAR_ccfence() \
          __asm__ __volatile__ ("" : : : "memory")

  #define MEMBAR_StoreStore() \
          __asm__ __volatile__ ("" : : : "memory")
          /* (x86 doesn't reorder stores) */
          /* (exception: "temporaral move" instructions, fast string ops) */

  /* NOTE: ACQUIRE and RELEASE barriers here are stronger than needed.
   * Intel clarified spec in 2010 that memory operations to typical WC memory
   * is ordered except for StoreLoad, so acquire and release need only be
   * compiler fences.  To ensure compatibility with previous code that might
   * have misused acquire or release when it really needed StoreLoad, this code
   * is not being changed.  Instead, new macros introduced (lowercased suffix)*/

  #define MEMBAR_acquire() \
          __asm__ __volatile__ ("" : : : "memory")

  #define MEMBAR_release() \
          __asm__ __volatile__ ("" : : : "memory")

 #ifdef __SSE2__
  #define MEMBAR_ACQUIRE() \
          __asm__ __volatile__ ("mfence" : : : "memory")
 #else
  #define MEMBAR_ACQUIRE() \
          __asm__ __volatile__ ("lock addl $0, 0(%%esp)" : : : "memory")
 #endif

 #ifdef __SSE2__
  #define MEMBAR_RELEASE() \
          __asm__ __volatile__ ("mfence" : : : "memory")
 #else
  #define MEMBAR_RELEASE() \
          __asm__ __volatile__ ("lock addl $0, 0(%%esp)" : : : "memory")
 #endif

 #ifdef __SSE2__
  #define MEMBAR_FULLSYNC() \
          __asm__ __volatile__ ("mfence" : : : "memory")
 #else
  #define MEMBAR_FULLSYNC() \
          __asm__ __volatile__ ("lock addl $0, 0(%%esp)" : : : "memory")
 #endif

  #define MEMBAR_LOCK_CAS(ptr) \
         (__extension__ ({ int result;                                     \
          __asm__ __volatile__ ("lock cmpxchgl %1,%2"                      \
                                : "=q"(result) : "r"(1),"m"(*(ptr)),"0"(0) \
                                : "memory", "cc"); result; }))

  #define MEMBAR_LOCK_RELEASE(ptr) \
          MEMBAR_RELEASE(); *(ptr) = 0


#elif defined(__GNUC__)

  /* GNU C 4 builtins */

  #define MEMBAR_ccfence() \
          __asm__ __volatile__ ("" : : : "memory")

  #define MEMBAR_StoreStore() \
          __sync_synchronize() /* anything better than full sync (expensive)? */

  #define MEMBAR_ACQUIRE() \
          __sync_synchronize() /* anything better than full sync (expensive)? */

  #define MEMBAR_RELEASE() \
          __sync_synchronize() /* anything better than full sync (expensive)? */

  #define MEMBAR_FULLSYNC() \
          __sync_synchronize()

  #define MEMBAR_LOCK_CAS(ptr) \
          (!!__sync_val_compare_and_swap((ptr), 0, 1))
          /* force bool context */

  #define MEMBAR_LOCK_RELEASE(ptr) \
          __sync_lock_release(ptr)


#else

  /* not implemented */
  #error membar macros not implemented for this platform


#endif



#ifndef MEMBAR_storestore
#define MEMBAR_storestore() \
        MEMBAR_StoreStore()
#endif
#ifndef MEMBAR_acquire
#define MEMBAR_acquire() \
        MEMBAR_ACQUIRE()
#endif
#ifndef MEMBAR_release
#define MEMBAR_release() \
        MEMBAR_RELEASE()
#endif
#ifndef MEMBAR_fullsync
#define MEMBAR_fullsync() \
        MEMBAR_FULLSYNC()
#endif
#ifndef MEMBAR_lock_cas
#define MEMBAR_lock_cas(ptr) \
        MEMBAR_LOCK_CAS(ptr)
#endif
#ifndef MEMBAR_lock_release
#define MEMBAR_lock_release(ptr) \
        MEMBAR_release(); *(ptr) = 0
#endif
#ifndef MEMBAR_ccfence
#define MEMBAR_ccfence() \
        MEMBAR_storestore()
#endif


/*
 * atomic compare and swap function for 32-bit values
 * atomically compares *ptr with cmp and replaces *ptr with val only if *ptr == cmp
 * returns 1 if *ptr was replaced by val, 0 otherwise
 */
int membar_cas_32(volatile uint32_t *ptr, unsigned int cmp, unsigned int val);
/*
 * atomic increment of unsigned int
 * returns value of *ptr before increment
 */
uint32_t membar_get_inc_uint(volatile uint32_t* ptr);

/* References
 *
 * Intel x86 chips
 * Intel(R) 64 and IA-32 Architectures Developer's Manual: Vol. 3A
* http://www.intel.com/content/www/us/en/processors/architectures-software-developer-manuals.html
* http://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-software-developer-vol-3a-part-1-manual.pdf
 *
 * POWER chips
 * http://www.ibm.com/developerworks/systems/articles/powerpc.html
 *
 * HP-UX on Itanium
 * http://h21007.www2.hp.com/portal/download/files/unprot/Itanium/inline_assem_ERS.pdf
 * http://h21007.www2.hp.com/portal/download/files/unprot/itanium/spinlocks.pdf
 */


#endif /* INCLUDED_MEMBAR_H */


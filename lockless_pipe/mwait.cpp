// Copyright © 2017, Oracle and/or its affiliates. All rights reserved.
// Dave Dice -- blogs.oracle.com/dave
//
// Demonstration of SPARC T7 MONITOR-MWAIT usage.
// 
// Build :
// g++ -O3 -std=gnu++17 mwait.cpp -lpthread -o mwait -m32
// g++ -O3 -std=gnu++17 mwait.cpp -lpthread -o mwait -m64

#define __STDC_FORMAT_MACROS 1

#include <stdint.h>
#include <inttypes.h>
#include <thread>
#include <math.h>
#include <cmath>
#include <chrono>
#include <iostream>
#include <vector>
#include <mutex>
#include <random>
#include <atomic>
#include <cassert>
#include <cstdalign>
#include <numeric>
#include <functional>
#include <algorithm>

#include <sys/time.h>
#include <poll.h>

#include <cxxabi.h>
#include <string.h>
#include <strings.h>

#define NOINLINE     __attribute__ ((noinline))

#include <sys/auxv.h>
#ifndef AV2_SPARC_MWAIT
// from /usr/include/sys/auxv_SPARC.h
#define AV2_SPARC_MWAIT 0x00000010 // mwait instruction and load/monitor ASIs
#endif

static int HasMWAIT() {
  uint_t ui[2];
  ui[0] = ui[1] = 0 ; 
  int err = getisax(ui, 2);
  return ui[1] & AV2_SPARC_MWAIT ; 
}

// beware that MWAIT admits spurious wakeups - best effort
// It uses the coherence protocol to monitor the line underlying "A". 
// Invalidation or timeout will cause MWAIT to return.
// More specifically, any displacement of the line causes MWAIT to return.
// Relately, external interrupts cause MWAIT to return. 

static int32_t MWAITWhileEqual32 (volatile int32_t * A, int32_t V, uint32_t nsTimo) { 
  int32_t rv = V ; 
  __asm__ __volatile__ (
    "lduwa [%[RA]]0x84, %%g1; xorcc %[RV],%%g1,%[RV]; bnz 1f; nop; wr %[RTIMO],%%asr28; 1:nop; " 
    : [RV] "+r" (rv) 
    : [RA] "r" (A), [RTIMO] "r" (nsTimo)
    : "g1", "memory") ;   
   return rv ; 
}

#if defined(_ILP32) 

// V8PlusA 32-bit mode : ILP32
// Consider using "+h" register constraint
// Must explicitly marshall 64-bit values and unmarshall into G/O registers

static int64_t MWAITWhileEqual64 (volatile int64_t * A, int64_t V, uint32_t nsTimo) { 
  int64_t rv = V ; 
  __asm__ __volatile__ (
    "sllx %H[RV],32,%%g2; srl %L[RV],0,%%g1; or %%g1,%%g2,%%g2; "
    "ldxa [%[RA]]0x84, %%g1; xorcc %%g2,%%g1,%%g2; bnz,pn %%xcc,1f; nop; wr %[RTIMO],%%asr28; 1:nop; " 
    "srlx %%g2,32,%H[RV]; mov %%g2,%L[RV]; " 
    : [RV] "+h" (rv) 
    : [RA] "r" (A), [RTIMO] "r" (nsTimo)
    : "g1", "g2", "memory") ;   
   return rv ; 
}

static_assert (sizeof(void *) == 4, "") ; 

#else

// LP64 V9

static int64_t MWAITWhileEqual64 (volatile int64_t * A, int64_t V, uint32_t nsTimo) { 
  int64_t rv = V ; 
  __asm__ __volatile__ (
    "ldxa [%[RA]]0x84, %%g1; xorcc %[RV],%%g1,%[RV]; bnz,pn %%xcc,1f; nop; wr %[RTIMO],%%asr28; 1:nop; " 
    : [RV] "+r" (rv) 
    : [RA] "r" (A), [RTIMO] "r" (nsTimo)
    : "g1", "memory") ;   
   return rv ; 
}

static_assert (sizeof(void *) == 8, "") ; 

#endif

// Ideally we'd use alignas(128), but that isn't handled gracefully by all compilers
#define CALIGNED __attribute__ ((aligned(128)))

struct _Aligner {
  CALIGNED int a [0] ;
} ;

template<typename T> struct Sequester {
  CALIGNED T V {} ;
  _Aligner Ax ;

  // Universal copy constructor ...
  template<typename U> Sequester( U && u ) : V( std::forward<U>(u) ) {} ;
  operator T () { return V ; }

  Sequester() {;}
  Sequester(const Sequester&) = delete;
};

// avoid false sharing and consequent spurious MWAIT returns by isolating the 
// waited-upon variable as the sole occupant of its underlying cache line.
static Sequester<volatile int32_t> WaitLocation {0} ; 

static volatile hrtime_t UpdateTime = 0 ; 

int main (int argc, char * argv[]) {
  setbuf (stdout, NULL) ; 

  if (!HasMWAIT()) { 
   printf ("CPU does not support MWAIT\n") ; 
   return 0 ;
  } 

  // Calibrate -- try to determine gethrtime() overheads and effective resolution. 
  // Consider using RDSTICK or RDTICK instead of gethrtime().  
  // PLT resolution will make the 1st call to gethrtime() rather slow.
  // We take the last sample as definitive, but the median would be better.  
  // On modern SPARC systems gethrtime() usually has a resolution of 5 nsecs.  
  hrtime_t dx = 0 ; 
  for (int k = 10 ; --k >= 0 ;) { 
    hrtime_t A = gethrtime() ; 
    dx = gethrtime() - A ; 
  }
  printf ("Expected gethrtime latency = %lld nsecs\n", dx) ; 

  // Manifest constant defined in SPARC implementation supplement ...
  const int MaxMWait = 1024*1024-64 ;      

  // Try some simple MWAIT operations with just a single thread.   
  // Vary the waiting duration
  // We expect these to terminate via timeout, although early spurious returns are allowed. 
  // That is, an occasional short wait is acceptable and expected.  
  // The SPARC manuals claim that MWAIT operations longer than MaxMWait should saturate 
  // or clamp at MaxMWait.  
  // Users would normally be expected to decompose long logical MWAIT operations into
  // a series of shorter MaxMWait epsiodes followed if necessary by an MWAIT for the remainder.
  // At that point, however, it might be better to consider surrendering the CPU via parking. 
  printf ("Simple single threaded trials\n") ; 
  WaitLocation = 11 ; 
  for (int k = 1 ; k < MaxMWait*8 ; k *= 2) { 
    printf ("timeout=%d : ", k) ; 
    for (auto s = 10 ; --s >= 0 ; ) { 
      const auto A = gethrtime() ; 
      const auto rv = MWAITWhileEqual32 (&WaitLocation.V, 11, k) ; 
      const auto WaitDuration = (gethrtime() - A) - dx ; 
      // "k" is the idealized expected waiting time, WaitDuration is the actual observed value.
      // WaitDuration = Duration(MWAIT(k)) 
      // Note that the CPU quantizes k mod 16 or 64, so there's minor disparity between
      // requested and actual for smaller values.  
      printf ("%" PRId64 " ", WaitDuration) ; 
    }
    printf ("\n") ; 
  }
  printf ("\n") ; 

  // Next, try a more interesting scenario where one thread writes to a location
  // MWAIT-ed up by a 2nd thread.  
  // Try to determine the response time.  
  printf ("Trials with 2 threads : waker and wakee\n") ; 
  WaitLocation = 0 ; 
  std::thread ([&] { 
    printf ("Updater running\n") ; 
    for (;;) {
      ::poll (NULL, 0, 1000) ; 
      UpdateTime = gethrtime() ; 
      WaitLocation = WaitLocation + 1 ; 
    }
  }).detach() ;

  const auto fini = gethrtime() + (20*1000LL*1000LL*1000LL) ; 
  for (;;) { 
    hrtime_t t = gethrtime() ; 
    if (t >= fini) break ; 

    int cmp = WaitLocation; 
    // The following is naively equivalent to : while (WaitLocation == cmp) ; 
    int rv = MWAITWhileEqual32 (&WaitLocation.V, cmp, MaxMWait) ; 
    hrtime_t Done = gethrtime() ;
    
    // Consider: subtract dx from observed store-to-mwait-return latency
    if (cmp != WaitLocation) {
      printf ("Latency=%" PRId64 "\n", Done-UpdateTime) ;
    }
 
    t = Done - t ; 
    // TODO : gather statistics on "t" values -- min, max, average, etc
    if (rv == 0) {
      if (t > (MaxMWait/2)) continue ; 
    }
    
    // report possibly short waits ...
    printf ("%d nsecs : rv=%d cmp=%d location=%d\n", int32_t(t), rv, cmp, int32_t(WaitLocation)) ; 
  }
  printf ("Done\n") ; 
  return 0 ; 
} 
 


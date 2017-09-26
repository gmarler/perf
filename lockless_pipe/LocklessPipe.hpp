#ifndef PIPE_LOCKLESS_PIPE_BUF_HPP
#define PIPE_LOCKLESS_PIPE_BUF_HPP

#include "Atom/Atom.hh"
#include "InterruptedInterface.hh"

#include <cerrno>                         // To get ETIMEDOUT
#include <cstring>
#include <exception>
#include <iomanip>
#include <iostream>
#include <pthread.h>
#include <stdint.h>                       // To get uint32_t, uint64_t
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

namespace Pipe {

  // Whether LocklessPipe class will be used by multiple processes or a
  // single process with multiple threads.  Only to be considered when
  // using the MutexWakeupPolicy template parameter.
  namespace SharedType {
    enum Type { PROCESS_SHARED, PROCESS_PRIVATE };
  }

  class NoWakeupPolicy;
  class NoWakeupUsecPolicy;


  /*
   * A Base class for passing callbacks into the pop() and timedpop() functions
   * of the lockless pipe. These callbacks are called just before the pipe goes
   * to sleep, employing either NoWakeupPolicy or MutexWakeupPolicy.
   */

  class PreWaitFunctor {
    public:
      virtual ~PreWaitFunction() { }
      virtual void operator()() = 0;
  };


  /*
   * A single reader / single writer pipe that allows data of varying length.
   * The implementation of read/write is lockless (uses read and write memory
   * barriers via MemoryFence).  Whether the class is composed of any locks
   * is dependent on the wakeup policy, which is a template parameter.
   *
   * The wakeup policy specifies how the writer wakes up the reader (when the
   * reader is waiting on an empty pipe).
   *
   * The class NoWakeupPolicy specifies that when the reader attempts a
   * blocking read (via the pop method) from the empty pipe, it will just poll
   * (and usleep) until the pipe has data.  In this case the wakeupReader
   * method is a NO-OP.
   */
  template<class Data, uint32_t PIPE_SIZE, class WakeupPolicy = NoWakeupPolicy>
  class LocklessPipe {
    /*
     * The amount of time read/write will sleep from when the pipe is
     * empty/full.
     * NOTE: There was a presumption that Solaris couldn't reliably sleep
     *       for less than the OS timeer interrupt.
     */
    enum { SLEEP_ON_BLOCK_USECS = 1000 };
    public:

    typedef Data DataHandle;
    /*
     * Constructor
     */
    explicit LocklessPipe();

    ~LocklessPipe() { }

    /*
     * push 'Data' into the pipe, blocking if there is not enough room.
     * \param value the 'Data' to push.
     * \throws Interrupted if the writer has been stopped.
     */
    void push(const Data& value) {
      void *ptr = startWrite(static_cast<uint32_t>(value.length()), true);
      if (ptr) {
        std::memcpy(ptr, value.data(), value.length());
        finishWrite();
      }
    }

    /*
     * pop 'Data' off the pipe, blocking if the pipe is empty.
     * \param value The 'Data' to pop (this wil be a handle over the raw data copied into buffer)
     * \param buffer pointer to a raw buffer where the popped item will be copied to
     * \param func pointer to the functor to call just before the pipe goes to sleep
     * \throws Interrupted if the reader has been stopped.
     */
    void pop(Data& data, void* buffer, PreWaitFunctor* func = 0);

    /*
     * Indicates if the writer is running.
     */
    bool isWriterRunning() const NO_THROW {
      return isWriterRunning_;
    }
    /*
     * Indicates if the reader is running.
     */
    bool isReaderRunning() const NO_THROW {
      return isReaderRunning_;
    }
    /*
     * Start the writer.
     * All subsequent calls to push calls will not throw Interrupt exceptions.
     * This call has no effect if the writer is already running
     */
    void startWriter() {
      isWriterRunning_ = true;
    }
    /*
     * Stop the writer.
     * All subsequent calls to push will throw Interrupted exceptions.
     */
    void stopWriter() {
      isWriterRunning_ = false;
    }
    /*
     * Start the reader.
     * All subsequent calls to pop will not throw Interrupted exceptions.
     * This call has no effect if the reader is already running.
     */
    void startReader() {
      isReaderRunning_ = true;
    }
    /*
     * Stop the reader.
     * All subsequent calls to pop will throw Interrupted exceptions.
     */
    void stopReader() {
      isReaderRunning = false;
      wakeup(wakeupPolicy_);
    }
    /*
     * Is there no room for a datum of size length in the buffer?
     * \param length The length of the item to put in the buffer.
     * \return true if it cannot fit, else false.
     */
    bool isFull(uint32_t length) const NO_THROW;
    /*
     * How many items are in the pipe?
     * \return the count of items in the pipe.
     */
    uint32_t count() const NO_THROW {
      return static_cast<uint32_t>(numWritten_ - numRead_);
    }
    /*
     * Number of items written so far
     * \return the number of items written
     */
    uint64_t numWritten() const NO_THROW {
      return numWritten_;
    }
    /*
     * Number of items read so far
     * \return The number of items read
     */
    uint64_t numRead() const NO_THROW {
      return numRead_;
    }
    /*
     * How many writes have failed (for non-blocking writes)
     * \return the number of failed writes
     */
    uint64_t numFailedWrites() const NO_THROW {
      return numFailedWrites_;
    }
    /*
     * How full is the pipe, expressed as a percentage?
     * \return percentage of pipe containing data
     */
    double percentFull() const NO_THROW {
      const uint32_t rptr = getPointer(readVPtr_);
      const uint32_t wptr = getPointer(writeVPtr_);
      const double percent = 100.0 * (rptr > wptr               ?
                                      PIPE_SIZE - (rptr - wptr) :
                                      wptr - rptr) / PIPE_SIZE;
      return percent;
    }
    /*
     * Is the buffer empty?
     * \return ture if it's empty, else false.
     */
    bool isEmpty() const NO_THROW {
      return (readVPtr_ == writeVPtr_);
    }
    /*
     * How many bytes can be stored in this pipe?
     * \return The size in bytes of the pipe's buffer.
     */
    uint32_t getPipeSize() const NO_THROW {
      return PIPE_SIZE;
    }
    /*
     * The max size of the lement pushed onto the pipe.
     * \return The size in bytes.
     */
    uint32_t getMaxPipeElementSize() const NO_THROW {
      return MAX_PIPE_ELEMENT_SIZE;
    }
    /*
     * Wakeup the reader if there is any data to be read. This only applies
     * if the WakeupPolicy template parameter is MutexWakeupPolicy. Otherwise,
     * this method is a NO-OP.
     */
    void wakeupReader() {
      if ( ! isEmpty() ) {
        wakeup(wakeupPolicy_);
      }
    }
    /*
     * print stats about the pipe
     */
    std::ostream& print(std::ostream& os) const;
    /*
     * Run a number of validation checks on the pipe to see if anything
     * obvious is corrupted.
     */
    bool validate(bool print = true) const NO_THROW;


    private:
    typedef uint64_t VersionedPointerType;

    // Statistics
    volatile uint64_t numRead_;
    volatile uint64_t numWritten_;
    uint64_t numFailedWrites_;

    // The 'stomp' data is on the inordinately pessimistic size. And also
    // misses the possibility that you'll stomp over the end of the buffer.
    const uint64_t stomp1;
    Atom<VersionedPointerType> readVPtr_;
    const uint64_t stomp2;
    Atom<VersionedPointerType> writeVPtr_;
    const uint64_t stomp3;
    Atom<VersionedPointerType> nextWriteVPtr_;
    const uint64_t stomp4;

    volatile bool isReaderRunning_;
    volatile bool isWriterRunning_;
    /*
     * The size of the element pushed onto the pipe will be the length param
     * to push + sizeof(length).
     * Current implementation never wraps around in the buffer for the same element.
     * So effectively, (element + sizeof(length) can not exceed half of pipe size.
     * Otherwise, deadlock will occur.
     * In adddition, pipe can not be pushed into as the total full state which would
     * increment the write pointer to be equal to the readptr (which is the empty condition).
     */
    static const uint32_t MAX_PIPE_ELEMENT_SIZE;
    static const uint64_t NUM_NANOSECONDS_PER_MICROSECOND;
    static const uint64_t NUM_NANOSECONDS_PER_SECOND;
    static const uint64_t NUM_MICROSECONDS_PER_SECOND;

    static const uint64_t STOMP;

    WakeupPolicy wakeupPolicy_;

    /* Pessimism. Guarantee buf_ is 32 aligned.
     * I'd like to guarantee 64 bit, but it gets very messy as different
     * compilers behave rather strangely with padding. In particular, on
     * gcc x686 compiler, AND NO OTHER, the size of this structure ends up
     * smaller on 32 bit than 64 bit
     */

    union
    {
      uint32_t pipe_align__unused__;
      char buf_[PIPE_SIZE];
    };

    void wakeup(const NoWakeupPolicy&) { }

    void wait(const NoWakeupPolicy&, uint64_t timeOut, PreWaitFunctor* func = 0);

    void wakeup(const NoWakeupUsecPolicy&) {  }

    void wait(const NoWakeupUsecPolicy&, uint64_t timeOut, PreWaitFunctor* func = 0);

    void clear();

    void* startWrite(uint32_t length, bool block = true);
    void finishWrite(void);

    bool isWrap(uint32_t length, uint32_t ptr) const NO_THROW {
        return (ptr + length > PIPE_SIZE);
    }

    uint32_t peek(char* buf) {
        const bool peek = true;
        return read(buf, peek, 0);
    }

    uint32_t read(char* buf, bool peek, uint64_t timeOut, PreWaitFunctor* func = 0);

    VersionedPointerType
    getVersionedPointer(uint32_t version, uint32_t pointer) const NO_THROW {
        VersionedPointerType ptr = pointer;
        ptr |= static_cast<uint64_t>(version) << 32;
        return ptr;
    }

    uint32_t getVersion(VersionedPointerType vPointer) const NO_THROW {
        return static_cast<uint32_t>(vPointer >> 32);
    }

    uint32_t getPointer(VersionedPointerType vPointer) const NO_THROW {
        return static_cast<uint32_t>(vPointer & 0xFFFFFFFF);
    }

  };

  // Initialize Constants
  template<class Data, uint32_t PIPE_SIZE, class WakeupPolicy>
  const uint64_t
  LocklessPipe<Data, PIPE_SIZE, WakeupPolicy>::NEVER_TIME_OUT
    = static_cast<uint64_t>(-1);

  template<class Data, uint32_t PIPE_SIZE, class WakeupPolicy>
  const uint64_t
  LocklessPipe<Data, PIPE_SIZE, WakeupPolicy>::NUM_NANOSECONDS_PER_MICROSECOND = 1000;

  template<class Data, uint32_t PIPE_SIZE, class WakeupPolicy>
  const uint64_t
  LocklessPipe<Data, PIPE_SIZE, WakeupPolicy>::NUM_NANOSECONDS_PER_SECOND = 1000000000;

  template<class Data, uint32_t PIPE_SIZE, class WakeupPolicy>
  const uint64_t
  LocklessPipe<Data, PIPE_SIZE, WakeupPolicy>::NUM_MICROSECONDS_PER_SECOND = 1000000;

  template<class Data, uint32_t PIPE_SIZE, class WakeupPolicy>
  const uint32_t
  LocklessPipe<Data, PIPE_SIZE, WakeupPolicy>::MAX_PIPE_ELEMENT_SIZE = PIPE_SIZE/2 - (sizeof(uint32_t) + 1);

  template<class Data, uint32_t PIPE_SIZE, class WakeupPolicy>
  const uint64_t
  LocklessPipe<Data, PIPE_SIZE, WakeupPolicy>::STOMP = 0xDEADBEEF;



  template<class Data, uint32_t PIPE_SIZE, class WakeupPolicy>
  inline
  LocklessPipe<Data, PIPE_SIZE, WakeupPolicy>::LocklessPipe():
    numFailedWrites_(0),
    stomp1(STOMP),
    stomp2(STOMP),
    stomp3(STOMP),
    stomp4(STOMP)
  {
    clear();

    isWriterRunning_ = true;
    isReaderRunning_ = true;
  }

  template<class Data, uint32_t PIPE_SIZE, class WakeupPolicy>
  inline
  void LocklessPipe<Data, PIPE_SIZE, WakeupPolicy>::pop(Data& value, void* buffer, PreWaitFunctor* func)
  {
      const bool peek = false;
      const uint32_t length = read(static_cast<char*>(buffer), peek, NEVER_TIME_OUT, func);
      if (length == 0)
      {
          // Something has gone *badly* wrong inside of read. Abort, abort!
          throw InternalReadError();
      }

      value = Data(static_cast<char*>(buffer), length);
  }

  template<class Data, uint32_t PIPE_SIZE, class WakeupPolicy>
  inline
  void LocklessPipe<Data, PIPE_SIZE, WakeupPolicy>::clear()
  {
      readVPtr_.set(0);
      writeVPtr_.set(0);
      nextWriteVPtr_.set(0);

      numRead_    = 0;
      numWritten_ = 0;
  }

  template<class Data, uint32_t PIPE_SIZE, class WakeupPolicy>
  inline
  void* LocklessPipe<Data, PIPE_SIZE, WakeupPolicy>::startWrite(uint32_t length, bool block)
  {
    /**
     * Check if the pipe is physically capable of holding this element.
     */
    if (length > MAX_PIPE_ELEMENT_SIZE)
    {
        std::cerr << "LocklessPipe: Data Passed in is too large! Length: " << length /* purecov: inspected */
                  << "\nPipe: \n"  /* purecov: inspected */
                  << *this;  /* purecov: inspected */
        return NULL;  /* purecov: inspected */
    }

    bool full;
    while ((full = isFull(length)) && isWriterRunning_ && block)
    {
      /*
       * For the MutexWakeupPolicy only:
       * If the writer calls wakeupReader rarely or not at all, we may get into
       * the case where the reader is hanging on cond-wait, and the
       * the pipe is full.  In this case, the writer must wakeup the reader, or
       * deadlock will ensue.
       */
      wakeup(wakeupPolicy_);
      usleep(SLEEP_ON_BLOCK_USECS);
    }

    if (!isWriterRunning_)
    {
      throw Interrupted();
    }

    if (!block && full)
    {
      return NULL;
    }

    uint32_t localWritePtr = getPointer(writeVPtr_);
    uint32_t localWriteVersion = getVersion(writeVPtr_);
    if (isWrap(static_cast<uint32_t>(sizeof(length)), localWritePtr))
    {
      localWritePtr = 0;
      localWriteVersion++;
    }
    uint32_t lengthPtr = localWritePtr;
    localWritePtr = localWritePtr + static_cast<uint32_t>(sizeof(length));

    if (isWrap(length, localWritePtr))
    {
      localWritePtr = 0;
      localWriteVersion++;
    }

    nextWriteVPtr_ = getVersionedPointer(localWriteVersion, localWritePtr + length);

    // Make sure that the nextWritePtr is written before the data is written into
    // the buffer.  This is important because the separateThreadPeek method looks
    // at the nextWriteVPtr_
    MF_write_sync();

    std::memcpy(&buf_[lengthPtr], static_cast<void *>(&length), sizeof(length));

    return &buf_[localWritePtr];
  }

  template<class Data, uint32_t PIPE_SIZE, class WakeupPolicy>
  inline
  void LocklessPipe<Data, PIPE_SIZE, WakeupPolicy>::finishWrite(void)
  {
    // numWritten_ must be incremented before the write_sync so that
    // the validate method can correctly sense corruption when numRead_ > numWritten_
    numWritten_++;

    // Make sure the data is written to the queue before the pointers are incremented.
    // This way if the queue reader sees incremented tick pointers, then the data will
    // be sure to be updated as well.
    MF_write_sync();

    writeVPtr_ = nextWriteVPtr_;
  }

  template<class Data, uint32_t PIPE_SIZE, class WakeupPolicy>
  inline
  void LocklessPipe<Data, PIPE_SIZE, WakeupPolicy>::wait(const NoWakeupPolicy&,
                                                         uint64_t timeOut,
                                                         PreWaitFunctor* func)
  {
      if (NEVER_TIME_OUT == timeOut)
      {
          while (isEmpty() && isReaderRunning_)
          {
              if(func != 0)
              {
                  (*func)();
              }
              usleep(SLEEP_ON_BLOCK_USECS);
          }
      }
      else
      {
          bool timedOut = false;
          if (timeOut < 1000 * NUM_NANOSECONDS_PER_MICROSECOND)
          {
              if (func != 0 && timeOut != 0)
              {
                  (*func)();
              }
              struct timespec timeOutSpec;
              TimeFraction<Second, NanoSecond> waitSeconds;
              waitSeconds.assign<NanoSecond>(timeOut);
              timeOutSpec.tv_sec  = static_cast<long>(waitSeconds.whole);
              timeOutSpec.tv_nsec = static_cast<long>(waitSeconds.fract);
              nanosleep(&timeOutSpec, NULL);
          }
          else
          {
              struct timeval currentTime;

              if (gettimeofday(&currentTime, NULL) != 0)
              {
                  throw Errno("returned from gettimeofday");
              }

              const uint64_t timeoutNSecs = timeOut + (currentTime.tv_sec*NUM_NANOSECONDS_PER_SECOND) +
                                                      (currentTime.tv_usec*NUM_NANOSECONDS_PER_MICROSECOND);

              while (isEmpty() && isReaderRunning_ && !timedOut)
              {

                  if (func != 0 && timeOut != 0)
                  {
                      (*func)();
                  }
                  // In theory this can return early due to things like signals, but I don't think that we really care.
                  usleep(SLEEP_ON_BLOCK_USECS);

                  if (gettimeofday(&currentTime, NULL) != 0)
                  {
                      throw Errno("returned from gettimeofday");
                  }

                  const uint64_t currtimeNSecs = (currentTime.tv_sec*NUM_NANOSECONDS_PER_SECOND)
                                               + (currentTime.tv_usec*NUM_NANOSECONDS_PER_MICROSECOND);
                  if (currtimeNSecs >= timeoutNSecs)
                  {
                      timedOut = true;
                  }
              }
          }
      }
  }

  template<class Data, uint32_t PIPE_SIZE, class WakeupPolicy>
  inline
  bool LocklessPipe<Data, PIPE_SIZE, WakeupPolicy>::isFull(uint32_t length) const NO_THROW
  {
    //in a simpler way- we could increment the writeVPtr_ and then check
    //isFull = (incrementedVersionedWritePtr >= readVPtr_)

    //however we may want to decrease the # of bits allocated to the version # in the future, which
    //will cause the version # to potentially wrap, so i will leave this.

    bool result = true;

    uint32_t localReadPtr  = getPointer(readVPtr_);
    uint32_t localWritePtr = getPointer(writeVPtr_);
    if (localReadPtr > localWritePtr)
    {
      if ((localWritePtr + (length + static_cast<uint32_t>(sizeof(length)))) < localReadPtr)
      {
          result = false;
      }
    }
    else // if (localReadPtr <= localWritePtr)
    {
      if (isWrap(static_cast<uint32_t>(sizeof(length)), localWritePtr))
      {
        if (length + static_cast<uint32_t>(sizeof(length)) < localReadPtr)
        {
            result = false;
        }
      }
      else
      {
        uint32_t tempIndex;
        tempIndex = localWritePtr + static_cast<uint32_t>(sizeof(length));
        if (isWrap(length, tempIndex))
        {
          if (length < localReadPtr)
          {
              result = false;
          }
        }
        else
        {
          result = false;
        }
      }
    }
    return result;
  }

  template<class Data, uint32_t PIPE_SIZE, class WakeupPolicy>
  inline
  bool LocklessPipe<Data, PIPE_SIZE, WakeupPolicy>::validate(bool print) const NO_THROW
  {
    bool valid = true;

    const uint64_t numRead = numRead_;
    const uint64_t numWritten = numWritten_;

    if (numRead > numWritten)
    {
      if (print)
      {
        std::cerr << "LocklessPipe Error: NumWritten/NumRead out of synch:"
                  << " Num Read: " << numRead << " Num Written: " << numWritten << "\n";
      }
      valid = false;
    }

    VersionedPointerType readVPtr = readVPtr_;
    VersionedPointerType writeVPtr = writeVPtr_;

    if (getVersion(writeVPtr) - getVersion(readVPtr) > 1)
    {
      if (print)
      {
        std::cerr << "LocklessPipe Error: Versions out of synch:"
                  << " Write Version: " << getVersion(writeVPtr) << " Read Version: " << getVersion(readVPtr) << "\n";
      }
      valid = false;
    }

    if (getPointer(writeVPtr) > PIPE_SIZE)
    {
      if (print)
      {
        std::cerr << "LocklessPipe Error: WritePtr out of bounds: " << writeVPtr << "\n";
      }
      valid = false;
    }

    if (getPointer(readVPtr) > PIPE_SIZE)
    {
      if (print)
      {
        std::cerr << "LocklessPipe Error: ReadPtr out of bounds: " << readVPtr << "\n";
      }
      valid = false;
    }

    if (getPointer(writeVPtr) == getPointer(readVPtr) && getVersion(writeVPtr) != getVersion(readVPtr))
    {
      if (print)
      {
        std::cerr << "LocklessPipe Error: Corrupt state:"
                  << " Write Version " << getVersion(writeVPtr) << " Write Pointer " << getPointer(writeVPtr)
                  << " Read Version " << getVersion(readVPtr) << " Read Pointer " << getPointer(readVPtr) << "\n";
      }
      valid = false;
    }

    if (stomp1 != STOMP || stomp2 != STOMP || stomp3 != STOMP || stomp4 != STOMP)
    {
      if (print)
      {
        StreamGuard guard(std::cerr);

        std::cerr << "LocklessPipe Error: Stomp variables have been corrupted: "
                  << std::hex << stomp1 << " : " << stomp2 << " : " << stomp3 << " : " << stomp4 << "\n";
      }
      valid = false;
    }

    if (print && !valid)
    {
      std::cerr << *this << std::endl;
    }

    return valid;
  }

  template<class Data, uint32_t PIPE_SIZE, class WakeupPolicy>
  inline
  std::ostream& LocklessPipe<Data, PIPE_SIZE, WakeupPolicy>::print(std::ostream& os) const
  {
    StreamGuard sg(os);
    const VersionedPointerType rptr = readVPtr_;
    const VersionedPointerType wptr = writeVPtr_;

    os << "\tNumber of items in pipe     " << count() << "\n"
       << "\tPercent full                " << percentFull() << "\n"
       << "\tPipe size (in bytes)        " << PIPE_SIZE << "\n"

       << "\tWriting to location         "
       << "(" << std::setw(8) << getVersion(wptr) << ", " << std::setw(8) << getPointer(wptr) << ")\n"

       << "\tReading from location       "
       << "(" << std::setw(8) << getVersion(rptr) << ", " << std::setw(8) << getPointer(rptr) << ")\n"

       << "\tNext write location         "
       << "(" << std::setw(8) << getVersion(nextWriteVPtr_) << ", " << std::setw(8) << getPointer(nextWriteVPtr_) << ")\n"

       << "\tNum Written                 " << numWritten_ << "\n"
       << "\tNum Read                    " << numRead_ << "\n"
       << "\tNum Failed Writes           " << numFailedWrites_ << "\n"
       << "\tPipe Writer is running      " << std::boolalpha << isWriterRunning_ << "\n"
       << "\tPipe Reader is running      " << std::boolalpha << isReaderRunning_ << "\n"
       << "\tWakeup Policy               " << wakeupPolicy_;

    return os;
  }


  template<class Data, uint32_t PIPE_SIZE, class WakeupPolicy>
  inline
  std::ostream& operator<<(std::ostream& os, const LocklessPipe<Data, PIPE_SIZE, WakeupPolicy>& pipe)
  {
    return pipe.print(os);
  }

  /*
   * No wakeup policy.  This policy allows the reader to simply sleep when the
   * pipe is empty.  No wakeups are required by the writer and so the wakeut1p
   * method is a nop.  The wait method will return false positives.
   */
  class NoWakeupPolicy
  {
    enum { SLEEP_ON_BLOCK_USECS = 10 };

    public:
      friend std::ostream& operator<<(std::ostream& os, const NoWakeupPolicy& /*policy*/)
      {
        return os << "No Wakeup Policy";
      }
  };

} // Pipe


#endif /* PIPE_LOCKLESS_PIPE_BUF_HPP */

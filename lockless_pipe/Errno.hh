#ifndef ERRNO_HH
#define ERRNO_HH

#include "StreamingException.h"

#include <errno.h>

/* An exception which can be thrown after errno is set
 *
 * Extra strings may be added to the exception by using
 * \code
 * throw Errno(a1) << a2 << a3; //etc
 * \endcode
 * in which case, the string " when " and a1, a2, a3 are appended to
 * the stored string (a1 is optional);
 *
 * \note
 * This has its own internal buffer for the error string, which is
 * technically slightly safer.
 */
class Errno : public StreamingException
{
  /* errno at the time of the throw */
  int errno_;

  /* Set when the user has added extra text to the message */
  bool has_extra_;

public:
  /* The default constructor remembers the error number */
  Errno() throw() :
          errno_(errno), has_extra_(false)
  {
    StreamingException::operator<<(std::strerror(errno_));
  }

  /* You may construct with any class that can be output */
  template <class T> explicit Errno(T const &v) throw() :
          errno_(errno), has_extra_(false)
  {
    StreamingException::operator<<(std::strerror(errno_));
    *this << v;
  }

  /* This constructor allows the user to explicitly specify errno */
  template <class T> Errno(int err, T const& v) throw() :
          errno_(err), has_extra_(false)
  {
    StreamingException::operator<<(std::strerror(errno_));
    *this << v;
  }

  /* Destructors shouldn't ever throw */
  ~Errno() throw() {}

  /* You may "output" extra text to the class */
  template <class T> Errno& operator<<(T const &v) throw()
  {
    if (!has_extra_)
    {
      has_extra_ = true;
      *this << " when ";
    }

    StreamingException::operator<<(v);

    return *this;
  }

  /* Get the errno in force at the time the exception was thrown */
  int getErrno() const throw()
  {
    return errno_;
  }
};

#endif /* ERRNO_HH */

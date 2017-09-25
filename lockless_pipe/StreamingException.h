#ifndef STREAMING_EXCEPTION_H
#define STREAMING_EXCEPTION_H

#include "Utility.h"

#include <cstdlib>
#include <cstring>
#include <exception>
#include <sstream>

/*
 * This class provides an exception class that allows an object with an
 * operator<< to be written into its message.  This provides stream
 * semantics for exceptions
 */
class StreamingException : public std::exception
{
  private:
    char message_[256];

  public:
    /*
     * This constructs the exception with
     * an initially zero-length (blank) message.
     */
    StreamingException() NO_THROW;

    /*
     * This constructs the exception with an initial
     * message of message.
     *
     * @param message the initial message of the exception
     */
    template<typename T> explicit StreamingException(const T& message) NO_THROW;

    /*
     * This method appends appendMessage to the exception's message.  It
     * returns the exception after the append operation.
     *
     * @param message the message to be appended
     * @return the exception after the append operation
     */
    template<typename T> StreamingException& operator<<(const T& appendMessage) NO_THROW;

    /*
     * This method returns the exception's message.
     *
     * @return the exception's message
     *
     * \note: what() must be no-throw
     */
    virtual const char* what() const throw();

    virtual ~StreamingException() throw();
};

inline StreamingException::StreamingException() NO_THROW
{
  message_[0] = 0;
}

template<typename T>
inline StreamingException::StreamingException(const T& message) NO_THROW
{
  message_[0] = 0;
  *this << message;
}

template<typename T>
inline StreamingException& StreamingException::operator<<(const T& appendMessage) NO_THROW
{
  std::size_t currMessageSize = std::strlen(message_);

  try
  {
    std::stringstream strStream;
    strStream << appendMessage;
    std::strncpy(message_ + currMessageSize,
                 strStream.str().c_str(),
                 sizeof(message_) - currMessageSize);
  }
  catch (...)
  {
    std::strncpy(message_ + currMessageSize,
                 "unexpected exception in StreamingException::"
                 "operator<<",
                 sizeof(message_) - currMessageSize);
  }
  message_[sizeof(message_) - 1] = 0;
  return *this;
}

inline const char* StreamingException::what() const throw()
{
    return message_;
}

inline StreamingException::~StreamingException() throw()
{
}

#endif /* STREAMING_EXCEPTION_H */

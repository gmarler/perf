#ifndef PIPE_INTERRUPTEDINTERFACE_HH
#define PIPE_INTERRUPTEDINTERFACE_HH

#include <stdexcept>

namespace Pipe {
  /*
   * Base class for all interrupted exceptions
   */
  class InterruptedInterface : public std::exception {
    public:
      InterruptedInterface() throw () {}
      char const * what() const throw () {
        return "InterruptedInterface";
      }
  };
} // Pipe

#endif /* PIPE_INTERRUPTEDINTERFACE_HH */

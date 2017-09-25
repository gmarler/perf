#ifndef UTILITY_H
#define UTILITY_H

#include <cstddef>     // for size_t
#include <exception>
#include <stdint.h>
#include <string>
#include <vector>

/*
 * Double macro to stringify things via preprocessor
 * Handy for the __LINE__ macro
 */

#define _QUOTE(_x) #_x
#define _MAKESTR(_x) _QUOTE(_x)

#if __cplusplus > 199711L
#define NO_THROW noexcept
#else
#define NO_THROW throw()
#endif

#define CT_ASSERT(idn, expr) \
  enum { idn = sizeof(Compile_Time_Assert<(expr)>) }

// NOTE: idn will be defined as the value if successful
#define CT_ASSERT_EQ(idn, expr1, expr2) \
  enum { idn = Compile_Time_Assert_Equal<(expr1), (expr2)>::MATCHED }

/*
 * Used for asserts at compile time. Can do compile-time checks
 * that, e.g. structs, are the correct size.
 */
template <bool> struct Compile_Time_Assert;
template <>     struct Compile_Time_Assert<true> { };

/*
 * Used for asserts at compile time. Can do compile-time checks
 * that match 2 number together to check.
 */
template <int X, int Y> struct  Compile_Time_Assert_Equal;
template <int X>        struct  Compile_Time_Assert_Equal<X,X> { enum { MATCHED = X }; };


inline char bool2YN(bool value) NO_THROW { return value ? 'Y' : 'N'; }
// OR inline char bool2YN(bool value) { return "NY"[value]; }
inline char bool2yn(bool value) NO_THROW { return value ? 'y' : 'n'; }
inline char bool2TF(bool value) NO_THROW { return value ? 'T' : 'F'; }
inline char bool2tf(bool value) NO_THROW { return value ? 't' : 'f'; }
inline char bool210(bool value) NO_THROW { return value ? '1' : '0'; }

/*
 * Network-endian bit-set (left to right).
 * \param addr Pointer to a piece of memory.
 * \param nr The bit to set (0 = lhs).
 */
inline void set_bit(void *addr, unsigned int nr) NO_THROW
{
  uint8_t mask = (uint8_t(1) << 7) >> (nr & 7);
  uint8_t *p = static_cast<uint8_t *>(addr) + (nr >> 3);

  *p |= mask;
}

/*
 * Network-endian bit-set (left to right).
 * \param addr Pointer to a piece of memory.
 * \param nr The bit to set (0 = lhs).
 * \param set true if we want to set it, else false.
 */
inline void set_bit(void *addr, unsigned int nr, bool set) NO_THROW
{
  const uint8_t mask((uint8_t(1) << 7) >> (nr & 7));
  uint8_t *p(static_cast<uint8_t *>(addr) + (nr >> 3));

  *p = (*p & ~mask) | (-set & mask);
}

/**
 * Network-endian bit-clear (left to right).
 * \param addr Pointer to a piece of memory.
 * \param nr The bit to clear (0 = lhs).
 */
inline void clr_bit(void *addr, unsigned int nr) NO_THROW
{
  uint8_t mask = (uint8_t(1) << 7) >> (nr & 7);
  uint8_t *p = static_cast<uint8_t *>(addr) + (nr >> 3);

  *p &= ~mask;
}

/**
 * Network-endian bit-test (left to right).
 * \param addr Pointer to a piece of memory.
 * \param nr The bit to check (0 = lhs).
 */
inline bool test_bit(const void *addr, unsigned int nr) NO_THROW
{
  uint8_t mask = (uint8_t(1) << 7) >> (nr & 7);
  uint8_t const *p = static_cast<uint8_t const *>(addr) + (nr >> 3);

  return (*p & mask) != 0;
}

/** This is a little utility class to clean up a wildcarded string
 * This will only permit ASCII characters in the string.
 *
 * \note Due to a mixture of the half-wittedness of the sun compiler
 * and an error in the standard, this is implemented as a pair of
 * functions, with non-defaulted parameters. Really, this should be
 * a template function with 2 arguments, and size, escape, search
 * and replace being defaulted template parameters.
 */
void strip_and_replace(
  char *to,                ///< The buffer for the result.
  std::size_t const size,  ///< Size of output buffer,
  char const *from,        ///< The source string
  char const escape,       ///< Escape character
  char const search,       ///< Character to replace
  char const replace,      ///< Character to replace above character
  bool const nostrip,      ///< If set to true, dont strip escapes
  const std::exception& e  ///< Exception to throw on error
  ) throw (std::exception);

inline void strip_and_replace(
  char *to,               ///< The buffer for the result.
  std::size_t size,       ///< Length of buffer
  char const *from,       ///< The source string
  const std::exception& e ///< Exception to throw on error
  ) throw (std::exception)
{
  strip_and_replace(to, size, from, '\\', '*', static_cast<char>(0xff), false, e);
}

std::string replace_slashes(std::string const &str,
                            std::string const &rep,
                            std::string::size_type start = 0);

std::string get_temp_file(const std::string& fname);

/**
 * This method works just like unix basename,
 * except it doesn't chop off the suffix.
 *
 * @param fname a string in filename format
 * @return everything after the last "/"
 */
std::string basename(const std::string &fname);

/*
 * This method is will round_robin a variable.  That is,
 * the method returns data after it has been incremented
 * unless that operation would have made it greater than
 * or equal to maxValue, in which case data will be zero.
 *
 * Not at all atomic.
 *
 * I made this templatized on two types because the
 * language is not smart enough to (say) cast an enumeration
 * value to a uint32_t when DataType is a uint32_t.
 *
 * @param data the value to increment
 * @param maxValue the upper bound on the value
 * @return the value after the operation
 */
template<typename DataType, typename MaxValueType>
inline DataType round_robin(DataType data, MaxValueType maxValue)
{
  ++data;

  /*
   * The cast is necessary in order to ensure that
   * maxValue won't be interpreted as the wrong type
   * (suppose DataType is a uint32_t, and an enumerated
   * value was passed in for maxValue.  Any given
   * compiler might decide that the enumerated
   * value in fact is signed, which will make
   * this method blow up if the unsigned
   * value is very large (since the signed
   * value will be negative and this
   * condition always will be true).
   */
  if (data >= static_cast<DataType>(maxValue))
  {
    data = 0;
  }

  return data;
}

/*
 * Helper function which splits the provided string into a list of substrings,
 * based on the delimiters.
 *
 * @param str The string to split
 * @param tokens The list of split strings, which is populated by this function
 * @param delimiters The delimiter to split on.
 */
void tokenize(const std::string& str,
              std::vector<std::string>& tokens,
              const std::string& delimiters=" ");

/*
 * Trims out whitespaces from a string.
 *
 * @param s the string to trim
 */
std::string trim(const std::string& s, const std::string& delimiters = " \a\b\f\n\r\t\v");

/**
 *  helper class to see if two types are the same. Can be used in CT_ASSERT
 *
 *  example:
 *     cout << same_type<float, int>::same << endl;
 *     cout << same_type<float, float>::same << endl;
 *
 *  produces:
 *     0
 *     1
 */
template <class T, class U> struct same_type { enum { same = 0 }; };
template <class T> struct same_type<T, T> { enum { same = 1 }; };

template<size_t Size> size_t align_up(size_t value)
{
  // Only works for powers of two.
  CT_ASSERT(PowerOfTwo, (Size & (Size - 1)) == 0);

  return (value + (Size - 1)) & ~(Size - 1);
}


#endif /* UTILITY_H */

/**
 * @file singleton.h
 * @brief Defines macros for creating singleton classes.
 */

#ifndef TRAA_BASE_SINGLETON_H
#define TRAA_BASE_SINGLETON_H

#include "base/disallow.h"

/**
 * @def STRICT_SINGLETON_DECLARE(TypeName)
 * @brief Macro for declaring a strict singleton class.
 *
 * This macro declares a strict singleton class with the specified TypeName.
 * The class will have a static instance() method that returns a pointer to the singleton instance.
 * The singleton instance is created on the first call to instance() and remains alive until program
 * termination. The TypeName class must have a private constructor and cannot be copied or assigned.
 */
#define STRICT_SINGLETON_DECLARE(TypeName)                                                         \
public:                                                                                            \
  static TypeName &instance() {                                                                    \
    static TypeName instance;                                                                      \
    return instance;                                                                               \
  }                                                                                                \
                                                                                                   \
private:                                                                                           \
  ~TypeName() = default;                                                                           \
  DISALLOW_IMPLICIT_CONSTRUCTORS(TypeName)

/**
 * @def SINGLETON_DECLARE(TypeName)
 * @brief Macro for declaring a singleton class.
 *
 * This macro declares a singleton class with the specified TypeName.
 * The class will have a static instance() method that returns a pointer to the singleton instance.
 * The singleton instance is created on the first call to instance() and remains alive until program
 * termination. The TypeName class can be copied or assigned.
 */
#define SINGLETON_DECLARE(TypeName)                                                                \
public:                                                                                            \
  static TypeName &instance() {                                                                    \
    static TypeName instance;                                                                      \
    return instance;                                                                               \
  }                                                                                                \
                                                                                                   \
private:                                                                                           \
  DISALLOW_COPY_AND_ASSIGN(TypeName)

#endif // TRAA_BASE_SINGLETON_H
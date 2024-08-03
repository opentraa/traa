#ifndef TRAA_BASE_DISALLOW_H
#define TRAA_BASE_DISALLOW_H

// A macro to disallow the copy constructor and operator= functions
// This should be used in the private: declarations for a class
#define DISALLOW_COPY_AND_ASSIGN(TypeName)                                                         \
  TypeName(const TypeName &) = delete;                                                             \
  void operator=(const TypeName &) = delete;

// A macro to disallow all the implicit constructors, namely the
// default constructor, copy constructor and operator= functions.
//
// This should be used in the private: declarations for a class
// that wants to prevent anyone from instantiating it. This is
// especially useful for classes containing only static methods.
#define DISALLOW_IMPLICIT_CONSTRUCTORS(TypeName)                                                   \
  TypeName() = default;                                                                            \
  DISALLOW_COPY_AND_ASSIGN(TypeName)

#endif // TRAA_BASE_DISALLOW_H
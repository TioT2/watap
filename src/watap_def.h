#ifndef __watap_def_h_
#define __watap_def_h_

#include <stack>
#include <iostream>
#include <fstream>
#include <array>
#include <vector>
#include <map>
#include <span>
#include <format>
#include <variant>
#include <optional>
#include <numeric>

/* Debug memory allocation support */ 
#if !defined(NDEBUG)
#  define _CRTDBG_MAP_ALLOC
#  include <crtdbg.h> 
#  define SetDbgMemHooks() \
     _CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF | \
     _CRTDBG_ALLOC_MEM_DF | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG))

/* Project namespace */
namespace watap
{
  /* Debug-only namespace, contains some utilitary things */
  namespace __debug
  {
    /* class for debug memory allocation hook support */
    static class mem_dummy
    {
    public: 
      /* Class constructor */
      mem_dummy( void )
      {
        //_CrtSetBreakAlloc(281); -- use for memory leak check
        SetDbgMemHooks(); 
      } /* End of '__Dummy' constructor */
    } Dummy;
  }
}

#endif /* _DEBUG */

#if !defined(NDEBUG)
#  ifdef _CRTDBG_MAP_ALLOC 
#    define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#  endif /* _CRTDBG_MAP_ALLOC */ 
#endif /* _DEBUG */

/* Project namespace */
namespace watap
{
  namespace common_types
  {
    /* Integer types */
    using INT8  = char;
    using INT16 = short;
    using INT32 = long;
    using INT64 = long long;

    /* Unsigned integer types */
    using UINT8  = unsigned char;
    using UINT16 = unsigned short;
    using UINT32 = unsigned long;
    using UINT64 = unsigned long long;

    /* Special type aliases */
    using BYTE  = UINT8;
    using WORD  = UINT16;
    using DWORD = UINT32;
    using QWORD = UINT64;

    /* Platform-dependent integer type aliases */
    using SIZE_T  = size_t;
    using SSIZE_T = ptrdiff_t;
    using INT     = int;
    using UINT    = unsigned int;

    /* Default floating point aliases */
    using FLOAT   = float;
    using DOUBLE  = double;
    using LDOUBLE = long double;

    /* Floating point types */
    using FLOAT32 = FLOAT;
    using FLOAT64 = DOUBLE;
    // using FLOAT80 = LDOUBLE; // hidden)))

    /* Short floating point type aliases */
    using FLT  = FLOAT;
    using DBL  = DOUBLE;
    using LDBL = LDOUBLE;

    /* Special types */
    using CHAR  = char;
    using VOID  = void;
    using WCHAR = wchar_t;

    /* Boolean type and basic constants */
    using BOOL = bool;
    inline constexpr BOOL TRUE = true;   // TRUE
    inline constexpr BOOL FALSE = false; // FALSE

    /* Global debug mode flag */
    inline constexpr BOOL IS_DEBUG =
    #if defined(NDEBUG)
      FALSE
    #else
      TRUE
    #endif // _NDEBUG
      ;
  } /* end of 'common_types' namespace */

  using namespace common_types;
} /* end of 'watap' namespace */

/***
 * Useful list of rust-like optional unwrapping functions.
 ***/

/* Setting */
#define WATAP_BRANCH(EXPRESSION, IF_TRUE, IF_FALSE) do { if (auto __tmp_val = (EXPRESSION)) { IF_TRUE(*__tmp_val); } else { IF_FALSE; } } while (FALSE)

// Meta setter
#define __WATAP_FN_SET_TO_HELPER(VALUE) = (VALUE)
#define WATAP_FN_SET_TO(REFERENCE) (REFERENCE) __WATAP_FN_SET_TO_HELPER

#define WATAP_CALL_OR_RETURN(FN, VALUE, RETURN_VALUE) WATAP_BRANCH(VALUE, FN, return (RETURN_VALUE))
#define WATAP_CALL_OR_BREAK(FN, VALUE) WATAP_BRANCH(VALUE, FN, break)

#define WATAP_SET_OR_RETURN(VARIABLE, VALUE, RETURN_VALUE) WATAP_BRANCH(VALUE, WATAP_FN_SET_TO(VARIABLE), return (RETURN_VALUE))
#define WATAP_SET_OR_BREAK(VARIABLE, VALUE) WATAP_BRANCH(VALUE, WATAP_FN_SET_TO(VARIABLE), break)

#endif // !defined(__watap_def_h_)

/* END OF 'watap_def.h' FILE */

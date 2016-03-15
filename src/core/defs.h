#ifndef __INCLUDE_GUARD_9C6FEC0F_BC10_4C29_9987_59603F8759A1
#define __INCLUDE_GUARD_9C6FEC0F_BC10_4C29_9987_59603F8759A1
#ifdef _MSC_VER
	#pragma once
#endif

typedef unsigned char   byte;
typedef unsigned short  ushort;
typedef unsigned int    uint;
typedef unsigned long   ulong;

#ifndef _ASSERT
#define _ASSERT(_X)
#endif


//_THREAD_LOCAL - for declaring static variables to be thread local
//_FORCE_INLINE - force the compiler to inline a function if it supports it
//_ALIGNOF - gets the alignment of a variable/structure

#ifdef __unix
#define _THREAD_LOCAL __thread
#define _FORCE_INLINE
#define _ALIGNOF __alignof__
#define modf modff
#define _InterlockedIncrement(_X) __sync_fetch_and_add(_X, 1)
#define _InterlockedDecrement(_X) __sync_fetch_and_sub(_X, 1)
#else
#define _THREAD_LOCAL __declspec(thread)
#define _FORCE_INLINE __forceinline
#define _ALIGNOF __alignof
#endif

//If we are not compiling for multi-thread, redefine _THREAD_LOCAL
#ifndef _OPENMP
#undef _THREAD_LOCAL
#define _THREAD_LOCAL
#endif

//disable acceleration structures if _NO_ACCELERATION is set to 1
#define ACC_STRUCT 1 //FIXME: use kd-tree (2) instead
#if NO_ACC==1
#undef ACC_STRUCT
#define ACC_STRUCT 0
#endif

// tell gcc to ignore certain warnings
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#pragma GCC diagnostic ignored "-Wreorder"

#endif //__INCLUDE_GUARD_9C6FEC0F_BC10_4C29_9987_59603F8759A1

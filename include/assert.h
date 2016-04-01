#ifndef _ASSERT_H_
#define _ASSERT_H_

#include <bwio.h>
#include <syscall.h>
#include <ts7200.h>

#ifdef _ASSERT

#define ASSERT(expr)    \
if (!(expr)) {\
    bwprintf(COM2, "\033[31massertion fail: %s, line %d\n\r", __FILE__, __LINE__);\
    bwprintf(COM2, "\033[0m");\
    Halt();\
}

#define ASSERTP(expr, format, ...)    \
if (!(expr)) {\
    bwprintf(COM2, "\033[31massertion fail: %s, line %d\n\r", __FILE__, __LINE__);\
    bwprintf(COM2, format, __VA_ARGS__);\
    bwprintf(COM2, "\033[0m");\
    Halt();\
}

#else
#define ASSERT(expr)
#define ASSERTP(expr, format, ...)
#endif

#endif

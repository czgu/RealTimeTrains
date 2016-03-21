#ifndef _ASSERT_H_
#define _ASSERT_H_

#include <bwio.h>
#include <syscall.h>
#include <ts7200.h>

#ifdef _ASSERT
#define ASSERT(expr)    \
if (!(expr)) {\
    bwprintf(COM2, "assertion fail: %s, line %d\n\r", __FILE__, __LINE__);\
    Halt();\
}

#else
#define ASSERT(expr)
#endif

#endif

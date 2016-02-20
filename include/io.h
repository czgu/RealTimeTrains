/*
 * io.h
 */
 
#ifndef _IO_H
#define _IO_H

typedef char *va_list;

#define __va_argsiz(t)	\
		(((sizeof(t) + sizeof(int) - 1) / sizeof(int)) * sizeof(int))

#define va_start(ap, pN) ((ap) = ((va_list) __builtin_next_arg(pN)))

#define va_end(ap)	((void)0)

#define va_arg(ap, t)	\
		 (((ap) = (ap) + __va_argsiz(t)), *((t*) (void*) ((ap) - __va_argsiz(t))))

#define COM1	0
#define COM2	1

#define ON	1
#define	OFF	0

typedef struct StringBuffer {
    char* string;
    int len;
    int max_len;
} StringBuffer;
void stringbuffer_init(StringBuffer* sb, char* str, int max_len);


// private
int stringbuffer_putc(StringBuffer *sb, char c);
int stringbuffer_putx(StringBuffer *sb, char c );
int stringbuffer_putstr(StringBuffer *sb, char *str );
int stringbuffer_putr(StringBuffer *sb, unsigned int reg );
void stringbuffer_putw(StringBuffer *sb, int n, char fc, char *bf );

// public
void pprintf(char *format, ... );

#endif

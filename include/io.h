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

int ioputx( int channel, char c );

int ioputstr( int channel, char *str );

int ioputr( int channel, unsigned int reg );

void ioputw( int channel, int n, char fc, char *bf );

void pprintf( int channel, char *format, ... );

#endif

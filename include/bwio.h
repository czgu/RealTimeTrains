/*
 * bwio.h
 */
 
#ifndef _BWIO_H
#define _BWIO_H

#ifdef _DEBUG
#define DEBUG_MSG(...) do { bwprintf(1, __VA_ARGS__); } while( 0 )
#else
#define DEBUG_MSG(...) do { } while( 0 )
#endif


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

#define UART_FAST 115200 
#define UART_SLOW 2400

/* http://www.marklin.com/tech/digital1/components/commands.html
 baud rate = 2400 
 start bits = 1 
 stop bits = 2 
 parity = none 
 word size = 8 bits 
 these settings are reflected in: 
                                       | no fifo
 0b 0000 0000 0000 0000 0000 0000 0 11 0 1 0 0 0 
                             8 bits ||   | stop bits 
*/
#define COM1_SETTINGS 0x68 
 
int bwsetfifo( int channel, int state );

int io_set_connection( int channel, int speed, int fifo );

int bwsetspeed( int channel, int speed );

void assert(int cond, char* msg);

int bwputc( int channel, char c );

int bwgetc( int channel );

int bwputx( int channel, char c );

int bwputstr( int channel, char *str );

int bwputr( int channel, unsigned int reg );

void bwputw( int channel, int n, char fc, char *bf );

void bwprintf( int channel, char *format, ... );

#endif

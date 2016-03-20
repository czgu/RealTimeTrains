/*
 * bwio.h
 */
 
#ifndef _BWIO_H
#define _BWIO_H

#include <io.h>

#ifdef _DEBUG
#define DEBUG_MSG(...) do { bwprintf(1, __VA_ARGS__); } while( 0 )
#else
#define DEBUG_MSG(...) do { } while( 0 )
#endif

int bwsetfifo( int channel, int state );

int bwsetspeed( int channel, int speed );

int bwputc( int channel, char c );

int bwgetc( int channel );

int bwputx( int channel, char c );

int bwputstr( int channel, char *str );

int bwputr( int channel, unsigned int reg );

void bwputw( int channel, int n, char fc, char *bf );

void bwprintf( int channel, char *format, ... );

#endif

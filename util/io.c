/*
 * ioio.c - busy-wait I/O routines for diagnosis
 *
 * Specific to the TS-7200 ARM evaluation board
 *
 */

#include <ts7200.h>
#include <io.h>
#include <syscall.h>

/*

int ioputx( int channel, char c ) {
	char chh, chl;

	chh = c2x( c / 16 );
	chl = c2x( c % 16 );
	ioputc( channel, chh );
	return ioputc( channel, chl );
}

int ioputr( int channel, unsigned int reg ) {
	int byte;
	char *ch = (char *) &reg;

	for( byte = 3; byte >= 0; byte-- ) ioputx( channel, ch[byte] );
	return ioputc( channel, ' ' );
}

int ioputstr( int channel, char *str ) {
	while( *str ) {
		if( ioputc( channel, *str ) < 0 ) return -1;
		str++;
	}
	return 0;
}

void ioputw( int channel, int n, char fc, char *bf ) {
	char ch;
	char *p = bf;

	while( *p++ && n > 0 ) n--;
	while( n-- > 0 ) ioputc( channel, fc );
	while( ( ch = *bf++ ) ) ioputc( channel, ch );
}

int iogetc( int channel ) {
	volatile int *flags, *data;
	unsigned char c;

	switch( channel ) {
	case COM1:
		flags = (int *)( UART1_BASE + UART_FLAG_OFFSET );
		data = (int *)( UART1_BASE + UART_DATA_OFFSET );
		break;
	case COM2:
		flags = (int *)( UART2_BASE + UART_FLAG_OFFSET );
		data = (int *)( UART2_BASE + UART_DATA_OFFSET );
		break;
	default:
		return -1;
		break;
	}
	while ( !( *flags & RXFF_MASK ) ) ;
	c = *data;
	return c;
}

int ioa2d( char ch ) {
	if( ch >= '0' && ch <= '9' ) return ch - '0';
	if( ch >= 'a' && ch <= 'f' ) return ch - 'a' + 10;
	if( ch >= 'A' && ch <= 'F' ) return ch - 'A' + 10;
	return -1;
}

char ioa2i( char ch, char **src, int base, int *nump ) {
	int num, digit;
	char *p;

	p = *src; num = 0;
	while( ( digit = ioa2d( ch ) ) >= 0 ) {
		if ( digit > base ) break;
		num = num*base + digit;
		ch = *p++;
	}
	*src = p; *nump = num;
	return ch;
}

void ioui2a( unsigned int num, unsigned int base, char *bf ) {
	int n = 0;
	int dgt;
	unsigned int d = 1;
	
	while( (num / d) >= base ) d *= base;
	while( d != 0 ) {
		dgt = num / d;
		num %= d;
		d /= base;
		if( n || dgt > 0 || d == 0 ) {
			*bf++ = dgt + ( dgt < 10 ? '0' : 'a' - 10 );
			++n;
		}
	}
	*bf = 0;
}

void ioi2a( int num, char *bf ) {
	if( num < 0 ) {
		num = -num;
		*bf++ = '-';
	}
	ioui2a( num, 10, bf );
}

void ioformat ( int channel, char *fmt, va_list va ) {
	char bf[12];
	char ch, lz;
	int w;

	
	while ( ( ch = *(fmt++) ) ) {
		if ( ch != '%' )
			ioputc( channel, ch );
		else {
			lz = 0; w = 0;
			ch = *(fmt++);
			switch ( ch ) {
			case '0':
				lz = 1; ch = *(fmt++);
				break;
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				ch = ioa2i( ch, &fmt, 10, &w );
				break;
			}
			switch( ch ) {
			case 0: return;
			case 'c':
				ioputc( channel, va_arg( va, char ) );
				break;
			case 's':
				ioputw( channel, w, 0, va_arg( va, char* ) );
				break;
			case 'u':
				ioui2a( va_arg( va, unsigned int ), 10, bf );
				ioputw( channel, w, lz, bf );
				break;
			case 'd':
				ioi2a( va_arg( va, int ), bf );
				ioputw( channel, w, lz, bf );
				break;
			case 'x':
				ioui2a( va_arg( va, unsigned int ), 16, bf );
				ioputw( channel, w, lz, bf );
				break;
			case '%':
				ioputc( channel, ch );
				break;
			}
		}
	}
}

void ioprintf( int channel, char *fmt, ... ) {
        va_list va;

        va_start(va,fmt);
        ioformat( channel, fmt, va );
        va_end(va);
}

void assert(int cond, char* msg) {
    #ifdef _ASSERT
    if (cond == 0) {
        ioputstr(COM2, "ASERTION FAILED: ");
        ioputstr(COM2, msg);
    }
    #endif
}

*/

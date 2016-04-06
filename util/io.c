/*
 * ioio.c - busy-wait I/O routines for diagnosis
 *
 * Specific to the TS-7200 ARM evaluation board
 *
 */

#include <ts7200.h>
#include <io.h>
#include <string.h>
#include <syscall.h>

void stringbuffer_init(StringBuffer* sb, char* str, int max_len) {
    sb->string = str;
    sb->len = 0;
    sb->max_len = max_len;
}

inline int stringbuffer_putc(StringBuffer *sb, char c) {
    sb->string[sb->len++] = c;

    return 0;
}

int stringbuffer_putx(StringBuffer *sb, char c) {
	char chh, chl;

	chh = c2x( c / 16 );
	chl = c2x( c % 16 );

    stringbuffer_putc(sb, chh);
	return stringbuffer_putc(sb, chl);
}

int stringbuffer_putr(StringBuffer *sb, unsigned int reg ) {
	int byte;
	char *ch = (char *) &reg;

	for( byte = 3; byte >= 0; byte-- ) stringbuffer_putx( sb, ch[byte] );
	return stringbuffer_putc( sb, ' ' );
}

int stringbuffer_putstr(StringBuffer *sb, char *str) {
    int copy_len = strlen(str);
    
    strncpy(sb->string + sb->len, str, copy_len);

    sb->len += copy_len;

	return 0;
}

void stringbuffer_putw(StringBuffer *sb, int n, char fc, char *bf ) {
	char ch;
	char *p = bf;

	while( *p++ && n > 0 ) n--;
	while( n-- > 0 ) stringbuffer_putc(sb, fc );
	while( ( ch = *bf++ ) ) stringbuffer_putc(sb, ch );
}

void pretty_format(char *fmt, va_list va, char* output_buffer, int buffer_size, int* out_len) {
    StringBuffer sb;    
    stringbuffer_init(&sb, output_buffer, buffer_size);

	char bf[12];
	char ch, lz;
	int w;
	
	while ( ( ch = *(fmt++) ) ) {
		if ( ch != '%' )
            stringbuffer_putc(&sb, ch);
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
				ch = a2i( ch, &fmt, 10, &w );
				break;
			}
			switch( ch ) {
			case 0: return;
			case 'c':
                stringbuffer_putc(&sb, va_arg( va, char ));
				break;
			case 's':
                stringbuffer_putw(&sb, w, 0, va_arg( va, char* ) );
				break;
			case 'u':
				ui2a( va_arg( va, unsigned int ), 10, bf );
                stringbuffer_putw(&sb, w, lz, bf);
				break;
			case 'd':
				i2a( va_arg( va, int ), bf );
                stringbuffer_putw(&sb, w, lz, bf);
				break;
			case 'x':
				ui2a( va_arg( va, unsigned int ), 16, bf );
                stringbuffer_putw(&sb, w, lz, bf);
				break;
			case '%':
                stringbuffer_putc(&sb, ch);
				break;
			}
		}
	}
    output_buffer[sb.len] = 0;
    *out_len = sb.len;
}

void pprintf(int channel, char *fmt, ... ) {
    #define OUTPUT_BUFFER_SIZE 200
    char output_buffer[OUTPUT_BUFFER_SIZE];
    int string_len;

    va_list va;
    va_start(va,fmt);

    pretty_format(fmt, va, output_buffer, OUTPUT_BUFFER_SIZE, &string_len);
    va_end(va);

    PutnStr(channel, output_buffer, string_len);
}

void spprintf(char* s, int* len, char *fmt, ...) {
    va_list va;

    int curr_len = *len;
    int delta_len = 0;

    va_start(va,fmt);
    pretty_format(fmt, va, s + curr_len, OUTPUT_BUFFER_SIZE, &delta_len);
    va_end(va);

    *len = curr_len + delta_len;
}

/*
void assert(int cond, char* msg) {
    #ifdef _ASSERT
    if (cond == 0) {
        ioputstr(COM2, "ASERTION FAILED: ");
        ioputstr(COM2, msg);
    }
    #endif
}
*/

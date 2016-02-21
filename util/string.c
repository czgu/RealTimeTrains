#include <string.h>
#include <bwio.h>


char c2x( char ch ) {
	if ( (ch <= 9) ) return '0' + ch;
	return 'a' + ch - 10;
}

int a2d( char ch ) {
    if( ch >= '0' && ch <= '9' ) return ch - '0';
    if( ch >= 'a' && ch <= 'f' ) return ch - 'a' + 10;
    if( ch >= 'A' && ch <= 'F' ) return ch - 'A' + 10;
    return -1;
}

char a2i( char ch, char **src, int base, int *nump ) {
    int num, digit;
    char *p;

    p = *src; num = 0;
    while( ( digit = a2d( ch ) ) >= 0 ) {
        if ( digit > base ) break;
        num = num*base + digit;
        ch = *p++;
    }
    *src = p; *nump = num;
    return ch;
}

void ui2a( unsigned int num, unsigned int base, char *bf ) {
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

void i2a( int num, char *bf ) {
    if( num < 0 ) {
        num = -num;
        *bf++ = '-';
    }
    ui2a( num, 10, bf );
}

int strcmp(char* a, char* b) {
    if (a == 0 && b == 0)
        return 0;
    if (a == 0 || b == 0)
        return 1;

    while(1) {
        if (*a == 0 && *b == 0) {
            return 0;
        }
        if (*a == 0) {
            return -1;
        } else if(*b == 0) {
            return 1;
        }
        if (*a++ != *b++)
            break;
    }
    if (*a > *b)
        return 1;
    return -1;
}

int strncmp(char* a, char* b, int len) {
    return substrcmp(a, b, 0, 0, len);
}

int substrcmp(char* a, char* b, int a_start, int b_start, int len) {
    // assume meaningful and correct input
    if (a == 0 && b == 0)
        return 1;
    if (a == 0 || b == 0)
        return 0;

    char* c = a + a_start;
    char* d = b + b_start;

    while(1) {
        if ((*c == 0 && *d == 0) || len == 0) {
            return 0;
        }
        if (*c == 0) {
            return -1;
        } else if(*d == 0) {
            return 1;
        }
        
        if (*c++ != *d++)
            break;
        len--;
    }
    if (*c > *d)
        return 1;
    return -1;
}

int strlen(const char* a) {
    // includes the null character
    int i = 0;
    while (a[i++] != 0);
    return i;
}

char* strcpy (char * destination,const char * source) {
    return strncpy(destination, source, strlen(source));
}

char* strncpy (char * destination,const char * source, int len) {
    memory_copy(source, len, destination, len);

    return destination;
}

int memory_copy(const void* src, int src_len, void* dest, int dest_len) {
    int copy_len = src_len < dest_len ? src_len : dest_len;

    int copy_int = 0;
    int copy_extra = copy_len;

    // must be able to do so when both src, dest are 4 byte aligned
    if (((unsigned int)src & 0x3) == 0 && ((unsigned int)dest & 0x3) == 0) {
        copy_int = copy_len >> 2; // div 4
        copy_extra = copy_len & 0x3; // mod 4
    }

    // int is 4 bytes
    int* s = (int*)src, *d = (int*)dest;
    while (copy_int-- > 0) {
        *d = *s;
        d++;
        s++;

    }

    if (copy_extra > 0) {
        char* sc = (char*)s, * dc = (char*)d;
        while (copy_extra-- > 0) {
            *dc  = *sc ;

            dc ++;
            sc ++;
        }
    }
    return copy_len;
}

void* memcpy(void* d, void* s, unsigned int size) {
    memory_copy(s, size, d, size);
    return d;
}

char* strcat(char* destination, const char * source) {
    return strncat(destination, source, strlen(source));
}

char* strncat(char* destination, const char* source, int len) {
    int dest_start = strlen(destination) - 1;

    memory_copy(source, len, destination + dest_start, len);

    return destination;
}




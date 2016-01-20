#ifndef STRING_H
#define STRING_H

int a2d( char ch );
char a2i( char ch, char **src, int base, int *nump );
void ui2a( unsigned int num, unsigned int base, char *bf );
void i2a( int num, char *bf );
int strcmp(char* a, char* b);
int substrcmp(char* a, char* b, int a_start, int b_start, int len);

#endif

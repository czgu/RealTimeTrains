#ifndef STRING_H
#define STRING_H


char c2x( char ch );
int a2d( char ch );
char a2i( char ch, char **src, int base, int *nump );
void ui2a( unsigned int num, unsigned int base, char *bf );
void i2a( int num, char *bf );

int memory_copy(const void* src, int src_len, void* dest, int dest_len);
void *memset(void *s, int c, unsigned int n);


int substrcmp(char* a, char* b, int a_start, int b_start, int len);

int strlen(const char* a);

int strcmp(char* a, char* b);
int strncmp(char* a, char* b, int len);

char* strcpy(char * destination,const char * source);
char* strncpy(char *destination, const char* source, int len);

char* strcat (char* destination, const char* source);
char* strncat (char* destination, const char* source, int len);
#endif

#ifndef PHANTOM_MALLOC
#define PHANTOM_MALLOC

#define _JBLEN 12 // Size of the jmp_buf on AMD64.

typedef int jmp_buf[_JBLEN];


int ph_setjmp (jmp_buf);
void ph_longjmp (jmp_buf, int);

#endif
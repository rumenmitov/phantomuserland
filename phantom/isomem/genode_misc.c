#include "genode_misc.h"
#include "stdio.h"

void _stub_print_func(char* func, char* file, char* line){
    ph_printf("STUB: %s\n", func);
}


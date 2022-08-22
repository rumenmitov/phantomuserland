#include <base/log.h>

extern "C" {
    int ph_putchar(char c){
        Genode::log("%c", c);
    }
}
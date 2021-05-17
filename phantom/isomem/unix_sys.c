void panic(const char *fmt, ...)
{
    va_list vl;

    // CI: this word is being watched by CI scripts. Do not change -- or change CI appropriately
    printf("\nPanic: ");
    va_start(vl, fmt);
    vprintf(fmt, vl);
    va_end(vl);

    //save_mem(mem, size);
    getchar();
    // CI: this word is being watched by CI scripts. Do not change -- or change CI appropriately
    printf("\nPress Enter from memcheck...");
    pvm_memcheck();
    //printf("\nPress Enter...");	getchar();
    exit(1);
}


void phantom_wakeup_after_msec(long msec)
{
    hal_sleep_msec(msec);
}

time_t time(time_t *);

//time_t fast_time(void)
long fast_time(void)
{
    return time(0);
}

extern int sleep(int);
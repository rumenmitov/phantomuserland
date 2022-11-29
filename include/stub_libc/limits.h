// From limits.h in old libc
#define CHAR_BIT 8

// From Genode's dde_linux header (kernel.h)
#define INT_MAX   ((int)(~0U>>1))
// #define UINT_MAX  (~0U)
// freetype needed 32 bit integer type. It looks like there was no options for it here
#define UINT_MAX  (0xffffffffu)
#define INT_MIN   (-INT_MAX - 1)
#define USHRT_MAX ((u16)(~0U))
#define LONG_MAX  ((long)(~0UL>>1))
#define SHRT_MAX  ((s16)(USHRT_MAX>>1))
#define SHRT_MIN  ((s16)(-SHRT_MAX - 1))
#define ULONG_MAX (~0UL)

#define SIZE_MAX  (ULONG_MAX)
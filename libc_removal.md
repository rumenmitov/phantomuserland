# Libc replacement

Phantom uses a number of libc functions. Original code contains implementations from other open source operating systems such as Mach, NewOS and OpenBSD. Genode also has libc implementation based on FreeBSD's libc. However, each call is performed via VFS and requires switching of the contexts. 

## Methodology

Firstly, we need to replace symbols so that they will not intersect with libc functions. The simplest solution is to add `ph_` prefix to the names and keep the rest of the definition.

Secondly, we need to chose the implementation. It should be either built-in Genode's, Phantom's or own implementation. Priority is given to Genode's native implementations. If there is no such implementation or the function requires a different behavior or if this function is not standard for mcirokernels, then Phantom's implementation should be used. If both implementations are not applicable or does not exist, own implementation using Genode's interfaces will be written.

Also, both solutions have certain risks. Implementation from Genode might behave differently from what Phantom OS components expect. Implementation from Phantom might have bugs and might be not vompatible with 64-bit architecture.

Following sections contain functions that are used by system's binary accroding to `nm`.

Each section contains table with following columns:

- `function`: function name
- `completed`: is the implementation complete and function is usable
- `ph_`: is prefix added to the function
- `Genode`: Is Genode's implementation used, or can it be implemented using Genode's interfaces
- `Phantom`: Is the implementation from old Phantom code used
- `comments`: additional comments regarding the implementation

## Memory

Allocation of memory using `malloc` should be used only for non-persistent memory. Hence the implementation using Genode's allocators should be provided.

| function    | completed | `ph_` | Genode | Phantom | comments |
| ----------- | --------- | ----- | ------ | ------- | -------- |
| `malloc`    | - | + | + | -  |  |
| `realloc`   | - | + | + | -  |  |
| `calloc`    | - | + | + | -  |  |

## Strings

Some of string operations are implemented in Genode, but the majority is not. Most of implementations are imported from the old Phantom code and located in `phantom/isomem/contrib` folder

| function    | completed | `ph_` | Genode | Phantom | comments |
| ----------- | --------- | ----- | ------ | ------- | -------- |
| `memcmp`    | - | + | + | -  |  |
| `memcpy`    | - | + | + | -  |  |
| `memmove`   | - | + | + | -  |  |
| `memset`    | - | + | + | -  |  |
| `strcat`    | - | + | - | +  |  |
| `strchr`    | - | + | - | +  |  |
| `strcmp`    | - | + | - | +  |  |
| `strcpy`    | - | + | - | +  | `copy_cstring()` is an alternative in Genode |
| `strdup`    | - | + | - | +  |  |
| `strerror_r`| - | + | - | -  | Should be replaced |
| `strlcpy`   | - | + | - | +  | `copy_cstring()` is an alternative in Genode |
| `strlen`    | - | + | - | +  |  |
| `strncmp`   | - | + | - | +  |  |
| `strncpy`   | - | + | - | +  |  |
| `strnlen`   | - | + | - | +  |  |
| `strrchr`   | - | + | - | +  |  |
| `strstr`    | - | + | - | +  |  |
| `strtol`    | - | + | - | +  |  |

# Input and output

| function    | completed | `ph_` | Genode | Phantom | comments |
| ----------- | --------- | ----- | ------ | ------- | -------- |
| `sscanf`    | - | - | - | - | - |
| `printf`    | - | - | - | - | - |
| `vprintf`   | - | - | - | - | - |
| `snprintf`  | - | - | - | - | - |
| `putc`      | - | - | - | - | - |
| `putchar`   | - | - | - | - | - |
| `puts`      | - | - | - | - | - |

## Misc


| function    | completed | `ph_` | Genode | Phantom | comments |
| ----------- | --------- | ----- | ------ | ------- | -------- |
| `pow`       | - | - | - | - | - |
| `qsort`     | - | - | - | - | - |
| `setjmp`    | - | - | - | - | - |
| `time`      | - | - | - | - | - |
| `nanosleep` | - | - | - | - | - |
| `main`      | - | - | - | - | - |

## Not considered

Following functions are not considered to be implemented since they are/were used by adapters from Phantom's HAL to Genode

- `main`
- `pthread_create`
- `pthread_exit`
- `pthread_mutex_destroy`
- `pthread_mutex_init`
- `pthread_mutex_lock`
- `pthread_mutex_unlock`
- `pthread_self`
- `sem_destroy`
- `sem_getvalue`
- `sem_init`
- `sem_post`
- `sem_wait`

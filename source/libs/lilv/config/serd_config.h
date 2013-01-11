
#ifndef _SERD_CONFIG_H_
#define _SERD_CONFIG_H_

#define SERD_VERSION "0.18.2"

#if defined(__APPLE__) || defined(__HAIKU__)
 #define HAVE_FMAX   1
 #define HAVE_FILENO 1
 #define HAVE_POSIX_MEMALIGN 1
#elif defined(__WIN32__)
 #define HAVE_FMAX   1
 #define HAVE_FILENO 1
#else
 #define HAVE_FMAX   1
 #define HAVE_FILENO 1
 #define HAVE_POSIX_MEMALIGN 1
 #define HAVE_POSIX_FADVISE  1
#endif

#endif /* _SERD_CONFIG_H_ */

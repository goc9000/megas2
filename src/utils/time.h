#ifndef _H_UTILS_TIME_H
#define _H_UTILS_TIME_H

#include <inttypes.h>
#include <time.h>

static inline int64_t timespec_delta_ns(struct timespec *later, struct timespec *earlier)
{
    return (later->tv_sec - earlier->tv_sec)*1000000000LL + later->tv_nsec - earlier->tv_nsec;
}

#endif

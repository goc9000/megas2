#include <stdio.h>
#include <stdarg.h>
#include <stdexcept>

#include "fail.h"

using namespace std;

void fail(const char *format, ...)
{
    char buf[16384];
    va_list args;

    va_start(args, format);
    vsprintf(buf, format, args);
    va_end(args);

    throw runtime_error(buf);
}

void info(const char *format, ...)
{
    char buf[16384];
    va_list args;

    va_start(args, format);
    vsprintf(buf, format, args);
    va_end(args);

    printf("%s\n", buf);
}

#ifndef SETLIMIT_H
#define SETLIMIT_H

#include <sys/time.h>
#include <sys/resource.h>

void setlimit (int resource, rlim_t n);

#endif

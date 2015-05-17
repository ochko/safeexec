#ifndef SAFEEXEC_H
#define SAFEEXEC_H

#include <sys/user.h>

#define INFINITY 0xFFFFFFFF

struct config
{
  rlim_t cpu;
  rlim_t memory;
  rlim_t aspace;
  rlim_t core;
  rlim_t stack;
  rlim_t fsize;
  rlim_t nproc;
  rlim_t clock;

  uid_t minuid;
  uid_t maxuid;
};

#endif

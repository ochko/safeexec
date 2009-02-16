#ifndef SAFEEXEC_H
#define SAFEEXEC_H

#define INFINITY 0xFFFFFFFF

struct config
{
  rlim_t cpu;
  rlim_t memory;
  rlim_t core;
  rlim_t stack;
  rlim_t fsize;
  rlim_t nproc;
  rlim_t clock;

  uid_t minuid;
  uid_t maxuid;
};

void idset (int v, char *section, unsigned int value);
int install (char *section);

#endif

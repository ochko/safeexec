#include <unistd.h>
#include <errno.h>

#include "setlimit.h"

void setlimit (int resource, rlim_t n)
{
  struct rlimit limit;

  limit.rlim_cur = limit.rlim_max = n;
  if (setrlimit (resource, &limit) < 0)
    {
      if (errno == EINTR)
        {
          error ("No permission to raise limit for %d to %d\n", resource, n);
        }
      else
        {
          error (NULL);
        }
    }
}

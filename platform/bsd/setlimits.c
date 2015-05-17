#include "setlimit.h"
#include "setlimits.h"

void setlimits (struct config profile)
{
  setlimit (RLIMIT_CORE, profile.core * 1024);
  setlimit (RLIMIT_STACK, profile.stack * 1024);
  setlimit (RLIMIT_FSIZE, profile.fsize * 1024);
  setlimit (RLIMIT_NPROC, profile.nproc);
  setlimit (RLIMIT_CPU, profile.cpu);

  /* socket buffer size in bytes */
  setlimit (RLIMIT_SBSIZE, 0);

  /* Address space(including libraries) limit */
  if (profile.aspace > 0)
    setlimit (RLIMIT_AS, profile.aspace * 1024);
}

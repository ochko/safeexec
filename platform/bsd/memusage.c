#include <unistd.h>
#include <paths.h>
#include <limits.h>
#include <fcntl.h>
#include <kvm.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <sys/resource.h>

#include "error.h"
#include "memusage.h"

static kvm_t *kd;

void memusage_init (void)
{
  const char *nlistf, *memf;
  char errbuf[_POSIX2_LINE_MAX];

  nlistf = NULL;
  memf = _PATH_DEVNULL;

  kd = kvm_openfiles(nlistf, memf, NULL, O_RDONLY, errbuf);

  if (kd == 0)
    error("memusage_init: %s", errbuf);
}

int memusage (pid_t pid)
{
  struct kinfo_proc *kp;
  int cnt = -1;

  kp = kvm_getprocs(kd, KERN_PROC_PID, pid, &cnt);
  if ((kp == NULL && cnt > 0) || (kp != NULL && cnt < 0))
    error("memusage: %s", kvm_geterr(kd));

  if (cnt == 1) {
    return kp->ki_rusage.ru_maxrss;
  }else{
    return -1;
  }
}

void memusage_close (void)
{
  kvm_close(kd);
}

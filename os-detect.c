#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <assert.h>

int terminate;
struct rusage rusage;

void timeout (int signal)
{
  assert (signal == SIGXCPU);
  assert (getrusage (RUSAGE_SELF, &rusage) == 0);
  terminate = 1;
}

void solve (void)
{
  int seconds;
  seconds = rusage.ru_utime.tv_sec;
  if (rusage.ru_utime.tv_usec >= 500000)
    seconds++;
  if (seconds > 2)
    printf ("-DLINUX_HACK");
}

int main (void)
{
  struct rlimit rlimit;
  terminate = 0;
  signal (SIGXCPU, timeout);
  rlimit.rlim_cur = 2;
  rlimit.rlim_max = RLIM_INFINITY;
  setrlimit (RLIMIT_CPU, &rlimit);
  while (terminate == 0);
  solve ();
  return (0);
}

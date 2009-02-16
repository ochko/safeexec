#define _GNU_SOURCE

#include "safe.h"

#define SAFE_LOOP(u,v) while (((u) = (v)), (((v) < 0) && (errno == EINTR)))

int safe_close (int fd)
{
  int v;
  SAFE_LOOP (v, close (fd));
  return (v);
}

int safe_dup2 (int oldfd, int newfd)
{
  int v;
  SAFE_LOOP (v, dup2 (oldfd, newfd));
  return (v);
}

int safe_sigaction (int n, struct sigaction *u, struct sigaction *v)
{
  int w;
  SAFE_LOOP (w, sigaction (n, u, v));
  return (w);
}

int safe_fcntl (int fd, int u, int v)
{
  int w;
  SAFE_LOOP (w, fcntl (fd, u, v));
  return (w);
}

int safe_signal (int n, void (*p) (int))
{
  struct sigaction u, v;
  u.sa_handler = p;
  sigemptyset (&u.sa_mask);
  u.sa_flags = 0;
  return (safe_sigaction (n, &u, &v));
}

#ifndef SAFE_H
#define SAFE_H
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
int safe_close (int);
int safe_dup2 (int, int);
int safe_sigaction (int, struct sigaction *, struct sigaction *);
int safe_fcntl (int, int, int);
int safe_signal (int, void (*)(int));
#endif

/* mooshak safeexec (safe execution environment)
 *
 * Idea:
 *   we use stdin and stdout to passing and receiving information to
 *   the program we are about to execute, and report statistics on
 *   FILE *redirect
 *
 */

#define _BSD_SOURCE             /* to include wait4 function prototype */
#define _POSIX_SOURCE           /* to include kill  function prototype */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <assert.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/select.h>

#include "safeexec.h"
#include "error.h"
#include "safe.h"
                                /* Fix for FreeBSD :<       */
#define SIGXFSZ         25      /* exceeded file size limit */

#define LARGECONST 4194304
#define SIZE          8192      /* buffer size for reading /proc/<pid>/status */
#define INTERVAL        67      /* about 15 times a second                    *
                                 * Is a good idea to use a prime number, as   *
                                 * the users will not notice it (much)        */
#define NICE_LEVEL 15

struct config profile = { 10, 32768, 0, 8192, 8192, 0, 60, 500, 65535 };
struct config *pdefault = &profile;

pid_t pid;                      /* is global, because we kill the proccess in alarm handler */
int mark;
int silent = 0;
char *usage_file = "/dev/null";
FILE *redirect;
char *chroot_dir = "/tmp";
char *run_dir = "run";

enum
{ OK, OLE, MLE, TLE, RTLE, RF, IE };    /* for the output statistics */
enum
{
  PARSE, INPUT1, INPUT16,
  INPUT2, INPUT4, INPUT8,
  ERROR, EXECUTE
};                              /* for the parsing "finite-state machine" */

char *names[] = {
  "UNKONWN",                    /*  0 */
  "SIGHUP",                     /*  1 */
  "SIGINT",                     /*  2 */
  "SIGQUIT",                    /*  3 */
  "SIGILL",                     /*  4 */
  "SIGTRAP",                    /*  5 */
  "SIGABRT",                    /*  6 */
  "SIGBUS",                     /*  7 */
  "SIGFPE",                     /*  8 */
  "SIGKILL",                    /*  9 */
  "SIGUSR1",                    /* 10 */
  "SIGSEGV",                    /* 11 */
  "SIGUSR2",                    /* 12 */
  "SIGPIPE",                    /* 13 */
  "SIGALRM",                    /* 14 */
  "SIGTERM",                    /* 15 */
  "SIGSTKFLT",                  /* 16 */
  "SIGCHLD",                    /* 17 */
  "SIGCONT",                    /* 18 */
  "SIGSTOP",                    /* 19 */
  "SIGTSTP",                    /* 20 */
  "SIGTTIN",                    /* 21 */
  "SIGTTOU",                    /* 22 */
  "SIGURG",                     /* 23 */
  "SIGXCPU",                    /* 24 */
  "SIGXFSZ",                    /* 25 */
  "SIGVTALRM",                  /* 26 */
  "SIGPROF",                    /* 27 */
  "SIGWINCH",                   /* 28 */
  "SIGIO",                      /* 29 */
  "SIGPWR",                     /* 30 */
  "SIGSYS",                     /* 31 */
};

void printstats (const char *format, ...)
{
  va_list p;
  if (silent == 1)
    return;
  va_start (p, format);
  vfprintf (redirect, format, p);
  va_end (p);
}

char *name (int signal)
{
  if (signal >= sizeof (names) / sizeof (char *))
    signal = 0;
  return (names[signal]);
}

int max (int a, int b)
{
  return (a > b ? a : b);
}

/* Kill the child process, noting that the child
   can already be a zombie (on  this case  errno
   will be ESRCH)
*/
void terminate (pid_t pid)
{
  int v;
  v = kill (-1, SIGKILL);
  if (v < 0)
    if (errno != ESRCH)
      error (NULL);
}

int miliseconds (struct timeval *tv)
{
  return ((int) tv->tv_sec * 1000 + (int) tv->tv_usec / 1000);
}

/* high resolution (microsecond) sleep */
void msleep (int ms)
{
  struct timeval tv;
  int v;
  do
    {
      tv.tv_sec = ms / 1000;
      tv.tv_usec = (ms % 1000) * 1000;
      v = select (0, NULL, NULL, NULL, &tv);
      /* The value of the timeout is undefined after the select returns */
    }
  while ((v < 0) && (errno == EINTR));
  if (v < 0)
    error (NULL);
}

int memusage (pid_t pid)
{
  char a[SIZE], *p, *q;
  int data, stack;
  int n, v, fd;

  p = a;
  sprintf (p, "/proc/%d/status", pid);
  fd = open (p, O_RDONLY);
  if (fd < 0){
    if (errno == ENOENT){
      return 0;
    } else {
      error (NULL);
    }
  }
  do
    n = read (fd, p, SIZE);
  while ((n < 0) && (errno == EINTR));
  if (n < 0)
    error (NULL);
  do
    v = close (fd);
  while ((v < 0) && (errno == EINTR));
  if (v < 0)
    error (NULL);

  data = stack = 0;
  q = strstr (p, "VmData:");
  if (q != NULL)
    {
      sscanf (q, "%*s %d", &data);
      q = strstr (q, "VmStk:");
      if (q != NULL)
        sscanf (q, "%*s %d\n", &stack);
    }

  return (data + stack);
}

void setlimit (int resource, rlim_t n)
{
  struct rlimit limit;

  limit.rlim_cur = limit.rlim_max = n;
  if (setrlimit (resource, &limit) < 0)
    error (NULL);
}

/* Validate the config options, call error () on error */
void validate (void)
{
  if (profile.cpu == 0)
    error ("Cpu time must be greater than zero");
  if (profile.memory >= LARGECONST)
    error ("Memory limit must be smaller than %u", LARGECONST);
  if (profile.core >= LARGECONST)
    error ("Core limit must be smaller than %u", LARGECONST);
  if (profile.stack >= LARGECONST)
    error ("Stack limit must be smaller than %u", LARGECONST);
  if (profile.fsize >= LARGECONST)
    error ("File size limit must be smaller than %u", LARGECONST);
  if (profile.nproc >= 65536)
    error ("Number of process(es) must be smaller than %u", 65536);
  if (profile.clock <= 0)
    error ("Wall clock time must be greater than zero");
  if (profile.clock >= LARGECONST)
    error ("Wall clock time must be smaller than %u", LARGECONST);
  if (profile.minuid < 500)
    error ("Lower uid limit is smaller than %u", 500);
  if (profile.maxuid >= 65536)
    error ("Upper uid limit must be smaller than %u", 65536);
  if (profile.minuid > profile.maxuid)
    error ("Lower uid limit is bigger than upper uid limit");
}

/* return NULL on failure, or argv + k
   where the command description starts */
char **parse (char **p)
{
  unsigned int *input1, *input2;
  char *function;
  int state;

  state = PARSE;
  if (*p == NULL)
    state = ERROR;
  else
    for (; state != ERROR;)
      {
        p++;
        if (*p == NULL)
          {
            state = ERROR;
            continue;
          }
        if (state == EXECUTE)
          break;
        switch (state)
          {
            case PARSE:
              state = INPUT1;
              function = *p;
              if (strcmp (*p, "--cpu") == 0)
                input1 = (unsigned int *) &profile.cpu;
              else if (strcmp (*p, "--mem") == 0)
                input1 = (unsigned int *) &profile.memory;
              else if (strcmp (*p, "--uids") == 0)
                {
                  input2 = (unsigned int *) &profile.minuid;
                  input1 = (unsigned int *) &profile.maxuid;
                  state = INPUT2;
                }
              else if (strcmp (*p, "--minuid") == 0)
                input1 = (unsigned int *) &profile.minuid;
              else if (strcmp (*p, "--maxuid") == 0)
                input1 = (unsigned int *) &profile.maxuid;
              else if (strcmp (*p, "--core") == 0)
                input1 = (unsigned int *) &profile.core;
              else if (strcmp (*p, "--nproc") == 0)
                input1 = (unsigned int *) &profile.nproc;
              else if (strcmp (*p, "--fsize") == 0)
                input1 = (unsigned int *) &profile.fsize;
              else if (strcmp (*p, "--stack") == 0)
                input1 = (unsigned int *) &profile.stack;
              else if (strcmp (*p, "--clock") == 0)
                input1 = (unsigned int *) &profile.clock;
              else if (strcmp (*p, "--exec") == 0)
                state = EXECUTE;
              else if (strcmp (*p, "--usage") == 0)
                state = INPUT4;
              else if (strcmp (*p, "--chroot") == 0)
                state = INPUT8;
              else if (strcmp (*p, "--rundir") == 0)
                state = INPUT16;
              else if (strcmp (*p, "--silent") == 0)
                {
                  silent = 1;
                  state = PARSE;
                }
              else
                {
                  fprintf (stderr, "error: Invalid option: %s\n", *p);
                  state = ERROR;
                }
              break;
            case INPUT4:
              usage_file = *p;
              state = PARSE;
              break;
            case INPUT8:
              chroot_dir = *p;
              state = PARSE;
              break;
            case INPUT16:
              run_dir = *p;
              state = PARSE;
              break;
            case INPUT2:
              if (sscanf (*p, "%u", input2) == 1)
                state = INPUT1;
              else
                {
                  fprintf (stderr,
                           "error: Failed to match the first numeric argument for %s\n",
                           function);
                  state = ERROR;
                }
              break;
            case INPUT1:
              if (sscanf (*p, "%u", input1) == 1)
                state = PARSE;
              else
                {
                  fprintf (stderr,
                           "error: Failed to match the numeric argument for %s\n",
                           function);
                  state = ERROR;
                }
              break;
            default:
              break;
          }
      }
  if (state == ERROR)
    return (NULL);
  else
    {
      assert (state == EXECUTE);
      validate ();
      return (p);
    }
}

void printusage (char **p)
{
  fprintf (stderr, "usage: %s <options> --exec <command>\n", *p);
  fprintf (stderr, "Available options:\n");
  fprintf (stderr, "\t--cpu     <seconds>           Default: %lu second(s)\n",
           pdefault->cpu);
  fprintf (stderr, "\t--mem     <kbytes>            Default: %lu kbyte(s)\n",
           pdefault->memory);
  fprintf (stderr, "\t--uids    <minuid> <maxuid>   Default: %u-%u\n",
           pdefault->minuid, pdefault->maxuid);
  fprintf (stderr, "\t--minuid  <uid>               Default: %u\n",
           pdefault->minuid);
  fprintf (stderr, "\t--maxuid  <uid>               Default: %u\n",
           pdefault->maxuid);
  fprintf (stderr, "\t--core    <kbytes>            Default: %lu kbyte(s)\n",
           pdefault->core);
  fprintf (stderr,
           "\t--nproc   <number>            Default: %lu proccess(es)\n",
           pdefault->nproc);
  fprintf (stderr, "\t--fsize   <kbytes>            Default: %lu kbyte(s)\n",
           pdefault->fsize);
  fprintf (stderr, "\t--stack   <kbytes>            Default: %lu kbyte(s)\n",
           pdefault->stack);
  fprintf (stderr,
           "\t--clock   <seconds>           Wall clock timeout (default: %lu)\n",
           pdefault->clock);
  fprintf (stderr,
           "\t--usage   <filename>          Report statistics to ... (default: stderr)\n");
  fprintf (stderr,
           "\t--chroot  <path>              Directory to chrooted (default: /tmp)\n");
}

void wallclock (int v)
{
  if (v != SIGALRM)
    error ("Signal delivered is not SIGALRM");
  mark = RTLE;
  terminate (pid);
}

int main (int argc, char **argv, char **envp)
{
  struct rusage usage;
  char **p;
  int status, mem;
  int tsource, ttarget;
  int v;

  redirect = stderr;
  safe_signal (SIGPIPE, SIG_DFL);

  tsource = time (NULL);
  p = parse (argv);
  if (p == NULL)
    {
      printusage (argv);
      return (EXIT_FAILURE);
    }
  else
    {
      /* Still missing: get an unused uid from interval */
      if (profile.minuid != profile.maxuid)
        {
          srand (time (NULL) ^ getpid ());
          profile.minuid += rand () % (profile.maxuid - profile.minuid);
        }

      if (strcmp (usage_file, "/dev/null") != 0)
        {
          redirect = fopen (usage_file, "w");
          chmod (usage_file, 0644);
          if (redirect == NULL)
            error ("Couldn't open redirection file\n");
        }

      if (signal (SIGALRM, wallclock) == SIG_ERR)
        error ("Couldn't install signal handler");

      if (alarm (profile.clock) != 0)
        error ("Couldn't set alarm");

      pid = fork ();
      if (pid < 0)
        error (NULL);
      if (pid == 0)
        {
          /* change to chroot dir */
          if (0 != chdir(chroot_dir))
            {
              kill (getpid (), SIGPIPE);
              error ("Cannot change to chroot dir");
            }

          /* chroot to judge dir  */
          if (0 != chroot(chroot_dir))
            {
              kill (getpid (), SIGPIPE);
              error ("Cannot chroot");
            }
          /* change to run dir */
          if (0 != chdir(run_dir))
            {
              kill (getpid (), SIGPIPE);
              error ("Cannot change to rundir");
            }

          if (setuid (profile.minuid) < 0)
            error (NULL);

          if (getuid () == 0)
            error ("Not changing the uid to an unpriviledged one is a BAD ideia");

          /* set priority */
          if (0 != setpriority(PRIO_USER,profile.minuid,NICE_LEVEL))
            {
              kill (getpid (), SIGPIPE);
              error (NULL);
            }

          /* Set Address space limit, 1 mbyte tolerancy (librarys also count!) */
          /*setlimit (RLIMIT_AS, (1024 + limit->memory) * 1024); */
          setlimit (RLIMIT_CORE, profile.core * 1024);
          setlimit (RLIMIT_STACK, profile.stack * 1024);
          setlimit (RLIMIT_FSIZE, profile.fsize * 1024);
          setlimit (RLIMIT_NPROC, profile.nproc);
          setlimit (RLIMIT_CPU, profile.cpu);

          /* Execute the program */
          if (execve (*p, p, envp) < 0)
            {
              kill (getpid (), SIGPIPE);
              error (NULL);
            }
        }
      else
        {
          if (setuid (profile.minuid) < 0)
            error (NULL);

          if (getuid () == 0)
            error ("Not changing the uid to an unpriviledged one is a BAD ideia");

          mark = OK;

          /* Poll at INTERVAL ms and determine the maximum *
           * memory usage,  exit when the child terminates */

          mem = 64;
          do
            {
              msleep (INTERVAL);
              mem = max (mem, memusage (pid));
              if (mem > profile.memory)
                {
                  terminate (pid);
                  mark = MLE;
                }
              do
                v = wait4 (pid, &status, WNOHANG | WUNTRACED, &usage);
              while ((v < 0) && (errno != EINTR));
              if (v < 0)
                error (NULL);
            }
          while (v == 0);

          ttarget = time (NULL);

          if (mark == MLE)
            printstats ("Memory Limit Exceeded\n");
          else if (mark == RTLE)
            printstats ("Time Limit Exceeded\n");
          else
            {
              if (WIFEXITED (status) != 0)
                {
                  if (WEXITSTATUS (status) != 0)
                    printstats ("Command exited with non-zero status (%d)\n",
                                WEXITSTATUS (status));
                  else
                    printstats ("OK\n");
                }
              else
                {
                  if (WIFSIGNALED (status) != 0)
                    {
                      /* Was killed for a TLE (or was it an OLE) */
                      if (WTERMSIG (status) == SIGKILL)
                        mark = TLE;
                      else if (WTERMSIG (status) == SIGXFSZ)
                        mark = OLE;
                      else if (WTERMSIG (status) == SIGHUP)
                        mark = RF;
                      else if (WTERMSIG (status) == SIGPIPE)
                        mark = IE;
                      else
                        printstats ("Command terminated by signal (%d: %s)\n",
                                    WTERMSIG (status),
                                    name (WTERMSIG (status)));
                    }
                  else if (WIFSTOPPED (status) != 0)
                    printstats ("Command terminated by signal (%d: %s)\n",
                                WSTOPSIG (status), name (WSTOPSIG (status)));
                  else
                    printstats ("OK\n");

                  if (mark == TLE)
                    {
                      /* Adjust the timings... although we know the child   *
                       * was been killed just in the right time seing 1.990 *
                       * as TLE when the limit is 2 seconds is anoying      */
                      usage.ru_utime.tv_sec = profile.cpu;
                      usage.ru_utime.tv_usec = 0;
                      printstats ("Time Limit Exceeded\n");
                    }
                  else if (mark == OLE)
                    printstats ("Output Limit Exceeded\n");
                  else if (mark == RTLE)
                    printstats ("Time Limit Exceeded\n");
                  else if (mark == RF)
                    printstats ("Invalid Function\n");
                  else if (mark == IE)
                    printstats ("Internal Error\n");
                }
            }
          printstats ("elapsed time: %d seconds\n", ttarget - tsource);
          printstats ("memory usage: %d kbytes\n", mem);
          printstats ("cpu usage: %d miliseconds\n",
                      miliseconds (&usage.ru_utime));
        }
    }
  fclose (redirect);

  return (EXIT_SUCCESS);
}

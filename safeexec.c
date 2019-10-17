/* safeexec (safe execution environment)
 *
 * Idea:
 *   we use stdin and stdout to passing and receiving information to
 *   the program we are about to execute, and report statistics on
 *   FILE *redirect
 */

#include <errno.h>
#include <ctype.h>
#include <sys/select.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/resource.h>

#include "error.h"
#include "safe.h"
#include "memusage.h"
#include "setlimit.h"
#include "safeexec.h"
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#define SIGXFSZ         25      /* exceeded file size limit(Fix for FreeBSD)  */
#define MAXMEM     8388608      /* recent version of Java need more memory    */
#define INTERVAL        61      /* about 16 times a second                    */
#define NICE_LEVEL      15
#define LARGECONST 4194304

struct config profile = { 1, 32768, 0, 0, 8192, 8192, 0, 10, 5000, 65535 };
struct config *pdefault = &profile;

pid_t pid;			/* is global, because we kill the proccess in alarm handler */
int mark;
int silent = 0;
FILE *redirect;
FILE *junk;
char *usage_file = "/dev/null";
char *error_file = "/dev/null";
char *chroot_dir = NULL;
char *run_dir = NULL;

enum{
  OK,   /* process finished normally	   */
  OLE,  /* output limit exceeded	   */
  MLE,  /* memory limit exceeded	   */
  TLE,  /* time limit exceeded		   */
  RTLE, /* time limit exceeded(wall clock) */
  RF,   /* invalid function		   */
  IE    /* internal error		   */
};
enum
  {
    PARSE, INPUT1, INPUT16,
    INPUT2, INPUT4, INPUT8,
    ERROR, EXECUTE, INPUT32
  };				/* for the parsing "finite-state machine" */

char *names[] = {
  "UNKONWN",      /*  0 */
  "SIGHUP",       /*  1 */
  "SIGINT",       /*  2 */
  "SIGQUIT",      /*  3 */
  "SIGILL",       /*  4 */
  "SIGTRAP",      /*  5 */
  "SIGABRT",      /*  6 */
  "SIGBUS",       /*  7 */
  "SIGFPE",       /*  8 */
  "SIGKILL",      /*  9 */
  "SIGUSR1",      /* 10 */
  "SIGSEGV",      /* 11 */
  "SIGUSR2",      /* 12 */
  "SIGPIPE",      /* 13 */
  "SIGALRM",      /* 14 */
  "SIGTERM",      /* 15 */
  "SIGSTKFLT",    /* 16 */
  "SIGCHLD",      /* 17 */
  "SIGCONT",      /* 18 */
  "SIGSTOP",      /* 19 */
  "SIGTSTP",      /* 20 */
  "SIGTTIN",      /* 21 */
  "SIGTTOU",      /* 22 */
  "SIGURG",       /* 23 */
  "SIGXCPU",      /* 24 */
  "SIGXFSZ",      /* 25 */
  "SIGVTALRM",    /* 26 */
  "SIGPROF",      /* 27 */
  "SIGWINCH",     /* 28 */
  "SIGIO",        /* 29 */
  "SIGPWR",       /* 30 */
  "SIGSYS",       /* 31 */
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
  v = kill (pid, SIGKILL);
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

/* Validate the config options, call error () on error */
void validate (void)
{
  if (profile.cpu == 0)
    error ("Cpu time must be greater than zero");
  if (profile.memory >= MAXMEM)
    error ("Memory limit must be smaller than %u", MAXMEM);
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
            else if (strcmp (*p, "--space") == 0)
              input1 = (unsigned int *) &profile.aspace;
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
            else if (strcmp (*p, "--error") == 0)
              state = INPUT32;
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
          case INPUT32:
            error_file = *p;
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
  fprintf (stderr, "\t--space   <kbytes>            Default: %lu kbyte(s)\n",
           pdefault->aspace);
  fprintf (stderr, "\t--uids    <minuid> <maxuid>   Default: %u-%u\n",
           pdefault->minuid, pdefault->maxuid);
  fprintf (stderr, "\t--minuid  <uid>               Default: %u\n",
           pdefault->minuid);
  fprintf (stderr, "\t--maxuid  <uid>               Default: %u\n",
           pdefault->maxuid);
  fprintf (stderr, "\t--core    <kbytes>            Default: %lu kbyte(s)\n",
           pdefault->core);
  fprintf (stderr, "\t--nproc   <number>            Default: %lu proccess(es)\n",
           pdefault->nproc);
  fprintf (stderr, "\t--fsize   <kbytes>            Default: %lu kbyte(s)\n",
           pdefault->fsize);
  fprintf (stderr, "\t--stack   <kbytes>            Default: %lu kbyte(s)\n",
           pdefault->stack);
  fprintf (stderr, "\t--clock   <seconds>           Wall clock timeout (default: %lu)\n",
           pdefault->clock);
  fprintf (stderr, "\t--usage   <filename>          Report statistics to ... (default: stderr)\n");
  fprintf (stderr, "\t--chroot  <path>              Directory to chrooted (default: /tmp)\n");
  fprintf (stderr, "\t--error   <path>              Print stderr to file (default: /dev/null)\n");
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
  int status, mem, skipped, memused;
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
      /* Get an unused uid */
      if (profile.minuid != profile.maxuid)
        {
          srand (time (NULL) ^ getpid ());
          profile.minuid += rand () % (profile.maxuid - profile.minuid);
        }

      if (strcmp (usage_file, "/dev/null") != 0)
        {
          redirect = fopen (usage_file, "w");
          chown (usage_file, profile.minuid, getgid());
          chmod (usage_file, 0640);
          if (redirect == NULL)
            error ("Couldn't open usage file\n");
        }

      /* stderr from user program is junk */
      junk = fopen (error_file, "w");
      if (junk == NULL)
        error ("Couldn't open junk file %s\n", error_file);
      if (strcmp (error_file, "/dev/null") != 0)
        {
          chown (error_file, profile.minuid, getgid());
          chmod (error_file, 0640);
        }

      if (setgid (profile.minuid) < 0)
        {
          if (errno == EPERM)
            {
              error ("Couldn't setgid due to permission");
            }
          else
            {
              error (NULL);
            }
        }
      if (setuid (profile.minuid) < 0)
        {
          if (errno == EPERM)
            {
              error ("Couldn't setuid due to permission");
            }
          else
            {
              error (NULL);
            }
        }


      if (getuid () == 0)
        error ("Not changing the uid to an unpriviledged one is a BAD ideia");

      if (signal (SIGALRM, wallclock) == SIG_ERR)
        error ("Couldn't install signal handler");

      if (alarm (profile.clock) != 0)
        error ("Couldn't set alarm");

      /* Fork new process */
      pid = fork ();
      if (pid < 0)
        error (NULL);

      if (pid == 0)
        /* Forked/child process */
        {
          /* Chrooting */
          if (chroot_dir != NULL)
            {
              if (0 != chdir(chroot_dir))
                {
                  kill (getpid (), SIGPIPE);
                  error ("Can not change to chroot dir");
                }
              if (0 != chroot(chroot_dir))
                {
                  kill (getpid (), SIGPIPE);
                  error ("Can not chroot");
                }
            }
          /* Change dir */
          if (run_dir != NULL)
            {
              if (0 != chdir(run_dir))
                {
                  kill (getpid (), SIGPIPE);
                  error ("Cannot change to rundir");
                }
            }

          dup2(fileno(junk), fileno(stderr));

          if (setuid (profile.minuid) < 0)
            error (NULL);

          /* Don't run as root */
          if (getuid () == 0)
            error ("Running as a root is not secure!");

          /* Set priority */
          if (0 != setpriority(PRIO_USER,profile.minuid,NICE_LEVEL))
            {
              kill (getpid (), SIGPIPE);
              error (NULL);
            }

          /* Set resource limits - memory, time etc */
          setlimits(profile);

          /* Execute the program */
          if (execve (*p, p, envp) < 0)
            {
              kill (getpid (), SIGPIPE);
              error (NULL);
            }
        }
      else
        /* Parent process */
        {
          if (setuid (profile.minuid) < 0)
            error (NULL);

          if (getuid () == 0)
            error ("Running as a root is not secure!");

          memusage_init ();

          mark = OK;

          /* Poll every INTERVAL ms and get the maximum   *
           * memory usage, exit when the child terminates */
          mem = 64;
          skipped = 0;
          memused = 0;
          do
            {
              msleep (INTERVAL);
              memused = memusage (pid);
              if (memused > -1)
                {
                  mem = max (mem, memused);
                }
              else
                { /* Can not read memory usage! */
                  skipped++;
                }

              if (skipped > 10)
                { /* process is already finished or something wrong happened */
                  terminate (pid);
                  mark = MLE;
                }

              if (mem > profile.memory)
                {
                  terminate (pid);
                  mark = MLE;
                }

              do
                v = wait4 (pid, &status, WNOHANG | WUNTRACED, &usage);
              //v = wait4 (pid, &status, WNOHANG | WUNTRACED | WCONTINUED | WEXITED, &usage);
              while ((v < 0) && (errno != EINTR));
              if (v < 0)
                error (NULL);
            }
          while (v == 0);

          memusage_close ();

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
                      /* We know the child has terminated at right time(OS did). *
                       * But seing 1.990 as TLE while limit 2.0 is confusing.    *
                       * So here is small adjustment for presentation.           */
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
          printstats ("cpu usage: %0.3f seconds\n",
                      (float) miliseconds (&usage.ru_utime) / 1000.0);
        }
    }
  fclose (redirect);

  return (EXIT_SUCCESS);
}

/* safeexec (safe execution environment)
 *
 * From Mooshak versions 1.4 and 1.6
 * niceness by http://github.com/ochko/safeexec
 * this version by http://github.com/daveagp
 *
 * see README
 */

#define _BSD_SOURCE		/* to include wait4 function prototype */
#define _POSIX_SOURCE		/* to include kill  function prototype */
#define _XOPEN_SOURCE 500       /* getpgid */
#define _GNU_SOURCE             /* CLONE_NEWIPC */

#include <sys/types.h>
#include <unistd.h>
#include <grp.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <ctype.h>
#include <sys/select.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <stdarg.h>
#include <sched.h>
#include <linux/version.h>

#include "safe.h"

#define SIZE          8192	/* buffer size for reading /proc/<pid>/status */
#define INTERVAL        67	/* about 15 times a second                    */

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

  rlim_t nfile;
  int niceness;

  gid_t gid;
  int uidplus;
  uid_t theuid;

  int report_fd;
  char *report_file;
  char *chroot_dir;
  char *exec_dir;
  char *env_vars;
  
  int share_newnet;
};


struct config profile_default
                      = { 10, 32768, 0, 8192, 0, 0, 60,
			  512, 16, 
			  1000, 10000, 0,
			  -1, NULL, NULL, NULL, NULL,
                          0 };

struct config profile;

pid_t pid;			/* is global, because we kill the proccess in alarm handler */
int mark;
int silent = 0;
FILE *redirect;

enum
  { OK, OLE, MLE, TLE, RTLE, RF, IE, RETNZ, TERM };	/* for the output statistics */
enum
  { EAT_INT, EAT_STRING, ERROR, EXECUTE, PARSE }; /* for the parsing */ 

char *names[] = {
  "UNKONWN",			/*  0 */
  "SIGHUP",			/*  1 */
  "SIGINT",			/*  2 */
  "SIGQUIT",			/*  3 */
  "SIGILL",			/*  4 */
  "SIGTRAP",			/*  5 */
  "SIGABRT",			/*  6 */
  "SIGBUS",			/*  7 */
  "SIGFPE",			/*  8 */
  "SIGKILL",			/*  9 */
  "SIGUSR1",			/* 10 */
  "SIGSEGV",			/* 11 */
  "SIGUSR2",			/* 12 */
  "SIGPIPE",			/* 13 */
  "SIGALRM",			/* 14 */
  "SIGTERM",			/* 15 */
  "SIGSTKFLT",			/* 16 */
  "SIGCHLD",			/* 17 */
  "SIGCONT",			/* 18 */
  "SIGSTOP",			/* 19 */
  "SIGTSTP",			/* 20 */
  "SIGTTIN",			/* 21 */
  "SIGTTOU",			/* 22 */
  "SIGURG",			/* 23 */
  "SIGXCPU",			/* 24 */
  "SIGXFSZ",			/* 25 */
  "SIGVTALRM",			/* 26 */
  "SIGPROF",			/* 27 */
  "SIGWINCH",			/* 28 */
  "SIGIO",			/* 29 */
  "SIGPWR",			/* 30 */
  "SIGSYS",			/* 31 */
};

void error (char *format, ...) {
  va_list p;
  if (format == NULL)
    error ("%s", strerror (errno));
  else {
    fprintf (stderr, "error %d (%s): ", errno, strerror(errno));
    va_start (p, format);
    vfprintf (stderr, format, p);
    va_end (p);
    fprintf (stderr, "\n");
  }
  exit (EXIT_FAILURE);
}

void printstats (const char *format, ...) { /* printf to the report file */ 
  va_list p;
  if (silent == 1)
    return;
  va_start (p, format);
  vfprintf (redirect, format, p);
  va_end (p);
}

char *name (int signal) {
  if (signal >= sizeof (names) / sizeof (char *))
    signal = 0;
  return (names[signal]);
}

int max (int a, int b) {
  return (a > b ? a : b);
}

void terminate (pid_t pid) {
  if (kill (pid, SIGKILL) < 0 && errno != ESRCH)
    error (NULL);
}

int milliseconds (struct timeval *tv)
{
  return ((int) tv->tv_sec * 1000 + (int) tv->tv_usec / 1000);
}

/* high resolution (microsecond) sleep */
void msleep (int ms) {
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

int memusage (pid_t pid) {
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

#ifdef LINUX_HACK
  /* Linux hack: in freebsd the process will   *
   * be killed exactly  after n  seconds. In   *
   * linux the behaviour depends on the kernel *
   * version (before 2.6 the process is killed *
   * after n+1 seconds, in 2.6 is after n.     */
  if (resource == RLIMIT_CPU)
    if (n > 0)
      n--;
#endif

  limit.rlim_cur = limit.rlim_max = n;
  if (setrlimit (resource, &limit) < 0)
    error (NULL);
}

/* Validate the config options, call error () on error */
void validate (void) {
  unsigned int LARGECONST;
  unsigned long HUGECONST;
  LARGECONST = 4194304;
  HUGECONST = 41943040;
  if (profile.cpu == 0)
    error ("Cpu time must be greater than zero");
  if (profile.memory >= HUGECONST)
    error ("Memory limit must be smaller than %u", HUGECONST);
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
  if (profile.uidplus >= LARGECONST)
    error ("uidplus must be smaller than %u", LARGECONST);
  if (profile.nfile > 1024)
    error ("File number limit must be no larger than %u", 1024);
  if (profile.niceness < 10 || profile.niceness > 19)
    error ("Niceness must be between 10 and 19");
  if (profile.gid < 1 || profile.gid > 65535)
    error ("Group id must be no larger than %u", 65535);
}

/* return NULL on failure, or argv + k
   where the command description starts */
char **parse (char **p)
{
  unsigned int *input1;
  char **string1;
  char *function;
  int state;

  state = PARSE;
  if (*p == NULL)
    state = ERROR;
  else
    for (; state != ERROR;) {
      p++;
      if (*p == NULL) {
	state = ERROR;
	continue;
      }
      if (state == EXECUTE)
	break;
      switch (state) {
      case PARSE:
	state = EAT_INT;
	function = *p;
	if (strcmp (*p, "--cpu") == 0)
	  input1 = (unsigned int *) &profile.cpu;
	else if (strcmp (*p, "--mem") == 0)
	  input1 = (unsigned int *) &profile.memory;
	else if (strcmp (*p, "--uidplus") == 0)
	  input1 = (unsigned int *) &profile.uidplus;
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
	else if (strcmp (*p, "--nfile") == 0)
	  input1 = (unsigned int *) &profile.nfile;
	else if (strcmp (*p, "--gid") == 0)
	  input1 = (unsigned int *) &profile.gid;
	else if (strcmp (*p, "--niceness") == 0)
	  input1 = (unsigned int *) &profile.niceness;
	else if (strcmp (*p, "--exec") == 0)
	  state = EXECUTE;
	else if (strcmp (*p, "--report_fd") == 0)
	  input1 = (unsigned int *) &profile.report_fd;
	else if (strcmp (*p, "--report_file") == 0) {
	  state = EAT_STRING;
	  string1 = (char **) &profile.report_file; 
	} else if (strcmp (*p, "--chroot_dir") == 0) {
	  state = EAT_STRING;
	  string1 = (char **) &profile.chroot_dir;
	} else if (strcmp (*p, "--exec_dir") == 0) {
	  state = EAT_STRING;
	  string1 = (char **) &profile.exec_dir; 
	} else if (strcmp (*p, "--env_vars") == 0) {
	  state = EAT_STRING;
	  string1 = (char **) &profile.env_vars; 
	} else if (strcmp (*p, "--share_newnet") == 0) {
	  profile.share_newnet = 1;
	  state = PARSE; 
	} else if (strcmp (*p, "--silent") == 0) {
	  silent = 1;
	  state = PARSE; 
	} else {
	  fprintf (stderr, "error: Invalid option: %s\n", *p);
	  state = ERROR; 
	}
	break;
      case EAT_STRING:
	*string1 = *p;
	state = PARSE;
	break;
      case EAT_INT:
	if (sscanf (*p, "%u", input1) == 1)
	  state = PARSE;
	else {
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
  else {
    assert (state == EXECUTE);
    validate ();
    return (p);
  }
}

void printusage (char **p)
{
  fprintf (stderr, "\nusage: %s <options> --exec <command>\n", *p);
  fprintf (stderr, "        **ATTENTION: read the security precautions in README**\n");
  fprintf (stderr, "Available options:\n");
  fprintf (stderr, "  --uidplus      <number>        Default: %u + pid = uid\n",
	   profile_default.uidplus);
  fprintf (stderr, "  --gid          <group id>      Default: %d group id\n",
	   ((int) profile_default.gid));
  fprintf (stderr, "  --cpu          <seconds>       Default: %lu seconds cpu time\n",
	   profile_default.cpu);
  fprintf (stderr, "  --clock        <seconds>       Default: %lu seconds wall time\n",
	   profile_default.clock);
  fprintf (stderr, "  --mem          <kbytes>        Default: %lu kbytes total memory\n",
	   profile_default.memory);
  fprintf (stderr, "  --core         <kbytes>        Default: %lu kbytes max core dump\n",
	   profile_default.core);
  /*  fprintf (stderr, "  --stack        <kbytes>        Default: %lu kbyte(s)\n",
	   profile_default.stack);
  This is undocumented as it is potentially confusing. It's per-process!
  */
  fprintf (stderr, "  --nproc        <number>        Default: %lu processes\n",
	   profile_default.nproc);
  fprintf (stderr, "  --fsize        <kbytes>        Default: %lu max kbytes written by all files\n",
	   profile_default.fsize);
  fprintf (stderr, "  --nfile        <number>        Default: %lu max open file pointers\n",
	   profile_default.nfile);
  fprintf (stderr, "  --niceness     <number>        Default: %d (19=min priority, 10=max)\n",
	   profile_default.niceness);
  fprintf (stderr, "  --chroot_dir   <dir>           Default: NULL (path starting with /)\n");
  fprintf (stderr, "  --exec_dir     <dir>           Default: NULL (relative to chroot_dir)\n");
  fprintf (stderr, "  --env_vars     \"X=Y\\nA=B\", PY  Default: inherit calling\n");
  fprintf (stderr, "  --report_file  <filename>      Default: stderr (output report, relative to .)\n");
  fprintf (stderr, "  --report_fd    <int>           Default: -1 (report to numbered file descriptor instead)\n");
  fprintf (stderr, "  --share_newnet                 Disable unshare(CLONE_NEWNET), e.g. for Java visualizer\n");
  fprintf (stderr, "Return: 0 - everything went ok; nonzero - internal errors, over-limit, etc.\n\n");

#ifdef LINUX_HACK
  fprintf (stderr, "Compiled with LINUX_HACK for RLIMIT_CPU\n");
#endif
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
  int unshare_flags;

  profile = profile_default;

  redirect = stderr;
  safe_signal (SIGPIPE, SIG_DFL);

  tsource = time (NULL);
  p = parse (argv);
  if (p == NULL) {
    printusage (argv);
    return (EXIT_FAILURE);
  }


  if (profile.env_vars != NULL) {
    if (strcmp(profile.env_vars, "PY") == 0) { 
      /* shorthand for needed Python environment variables, assuming that
	 the necessary executables, libraries, etc. are in the root of jail */
      char* xnewenvp[] = { "PYTHONIOENCODING=utf-8", "PYTHONHOME=/", "PYTHONPATH=/static", NULL };
      envp = xnewenvp;
    }
    else {
      char * newenvp[100];
      char ** iter = newenvp;
      *iter = strtok (profile.env_vars,"\n");
      while (*iter != NULL) { 
	iter++;
	*iter = strtok (NULL, "\n");
      } 
      envp = newenvp;
    }
  }

  if (profile.report_file != NULL) {
    redirect = fopen (profile.report_file, "w");
    if (redirect == NULL)
      error ("Couldn't open redirection file\n");
  }
  else if (profile.report_fd != -1) {
    redirect = fdopen(profile.report_fd, "w");
    if (redirect == NULL)
      error ("Couldn't open redirection file descriptor\n");
  }
      
  profile.theuid = profile.uidplus + getpid();

  pid = fork ();
  if (pid < 0)
    error (NULL);

  if (pid == 0) { /* FORK: slave process that will run the submitted code */

    setlimit (RLIMIT_AS, profile.memory * 1024); 
    setlimit (RLIMIT_DATA, profile.memory * 1024); 
    setlimit (RLIMIT_CORE, profile.core * 1024);
    setlimit (RLIMIT_STACK, profile.stack * 1024);
    setlimit (RLIMIT_FSIZE, profile.fsize * 1024);
    setlimit (RLIMIT_CPU, profile.cpu);
    setlimit (RLIMIT_NOFILE, profile.nfile);
    setlimit (RLIMIT_NPROC, profile.nproc + 1);  /* 1 required by setuid */
    
    if (profile.chroot_dir != NULL && 0 != chdir(profile.chroot_dir)) {
      error ("Could not chdir to chroot dir [%s] while in [%s]\n", profile.chroot_dir, getcwd(NULL, 0));
      kill (getpid (), SIGPIPE);
    }
    
    if (profile.chroot_dir != NULL && chroot(".") != 0) {
      error ("Cannot chroot while in [%s]\n", getcwd(NULL, 0));
      kill (getpid (), SIGPIPE);
    }
    
    if (profile.exec_dir != NULL && 0 != chdir(profile.exec_dir)) {
      error ("Could not chdir to exec_dir [%s] while in [%s]\n", profile.exec_dir, getcwd(NULL, 0));
      kill (getpid (), SIGPIPE);
    }
    
    if (0 != setpriority(PRIO_PROCESS, getpid(), profile.niceness)) {
      error ("Could not set priority");
      kill (getpid (), SIGPIPE);
    }

    /* unshare everything you can! */
    unshare_flags = CLONE_FILES | CLONE_FS | CLONE_NEWNS;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19)
    unshare_flags |= CLONE_NEWIPC | CLONE_NEWUTS;
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
    if (profile.share_newnet == 0)
       unshare_flags |= CLONE_NEWNET;
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
    unshare_flags |= CLONE_SYSVSEM;
#endif

    if (unshare (unshare_flags) < 0)
       error ("unshare failed\n");
    /* unshare everything */
    
    if (setsid () < 0)
          error ("setsid failed\n");
    /* new session and therefore also, new process group */

    if (setgid (profile.gid) < 0 || getgid () != profile.gid || profile.gid == 0)
      error ("setgid failed\n");
    
    if (setgroups (0, NULL) < 0 || getgroups (0, NULL) != 0)
      error ("setgroups failed\n");
    
    if (setuid (profile.theuid) < 0 || getuid() != profile.theuid || profile.theuid == 0)
      error ("setuid failed\n");
    
    if (execve (*p, p, envp) < 0) {
      error ("execve error\n");
      kill (getpid (), SIGPIPE);
    }
    /* goodbye process! execve never returns. */
  }
  
  /* FORK: supervisor process */
    
  if (signal (SIGALRM, wallclock) == SIG_ERR)
    printstats ("Couldn't install signal handler\n");
  
  else if (alarm (profile.clock) != 0)
    printstats ("Couldn't set alarm\n");
  
  else {
    mark = OK;
    
    /* Poll at INTERVAL ms and determine the maximum *
     * memory usage,  exit when the child terminates */
    
    mem = -1;
    do {
      msleep (INTERVAL);
      mem = max (mem, memusage (pid));
      if (mem > profile.memory) {
        terminate (pid);
        mark = MLE;
      }
      do
        v = wait4 (pid, &status, WNOHANG | WUNTRACED, &usage);
      while ((v < 0) && (errno != EINTR));
      if (v < 0)
        error (NULL);
    } while (v == 0);
    
    ttarget = time (NULL);
    
    if (mark == MLE)
      printstats ("Memory Limit Exceeded\n");
    else if (mark == RTLE)
      printstats ("Time Limit Exceeded\n");
    else {
      if (WIFEXITED (status) != 0) {
        if (WEXITSTATUS (status) != 0) {
          if (mark == OK)
            mark = RETNZ;
          printstats ("Command exited with non-zero status (%d)\n",
                      WEXITSTATUS (status));
        }
        else
          printstats ("OK\n");
      }
      else {
        if (WIFSIGNALED (status) != 0) {
          /* Was killed for a TLE (or was it an OLE) */
          if (WTERMSIG (status) == SIGKILL)
	    mark = TLE;
          else if (WTERMSIG (status) == SIGXFSZ)
            mark = OLE;
          else if (WTERMSIG (status) == SIGHUP)
            mark = RF;
          else if (WTERMSIG (status) == SIGPIPE)
            mark = IE;
          else {
            if (mark == OK)
              mark = TERM;
            printstats ("Command terminated by signal (%d: %s)\n",
                        WTERMSIG (status),
                        name (WTERMSIG (status)));
          }
        }
        else if (WIFSTOPPED (status) != 0) {
          if (mark == OK)
            mark = TERM;
          printstats ("Command terminated by signal (%d: %s)\n",
                      WSTOPSIG (status), name (WSTOPSIG (status)));
        }
        else
          printstats ("OK\n");
        
        if (mark == TLE) {
          /* Adjust the timings... although we know the child   *
           * was been killed just in the right time seeing 1.99 *
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
    if (mem != -1) /* -1: died too fast to measure */
      printstats ("memory usage: %d kbytes\n", mem);
    printstats ("cpu usage: %0.3f seconds\n",
                (float) milliseconds (&usage.ru_utime) / 1000.0);
  } 

  /* Clean up any orphaned descendant processes. kill everything with the target user id.
     The only reliable way to do this seems to be kill(-1) from that user.
     We also have to use setgid to change the euid away from 0. */
  if (setgid (profile.gid) < 0 || getgid () != profile.gid || profile.gid == 0)
    printstats ("cleanup setgid failed\n");
  else if (setgroups (0, NULL) < 0 || getgroups (0, NULL) != 0)
    printstats ("cleanup setgroups failed\n");
  else if (setuid (profile.theuid) < 0 || getuid() != profile.theuid || profile.theuid == 0)
    printstats ("cleanup setuid failed\n");
  
  if (getuid() != 0 && getuid() == profile.theuid 
      && geteuid() != 0 && geteuid() == profile.theuid
      && getgid() != 0 && getgid() == profile.gid) {
    kill(-1, SIGKILL);
  }
  else {
    printstats ("not safe to kill: uid %d, euid %d, gid %d, target uid %d, target gid %d\n", 
                getuid(), geteuid(), getgid(), profile.theuid, profile.gid);
  }

  fclose (redirect);

  if (mark == OK)
    return (EXIT_SUCCESS);
  else
    return (EXIT_FAILURE);
}

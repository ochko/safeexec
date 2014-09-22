#include <unistd.h>
#include <stdio.h>

int main (void)
{
  pid_t pid;
  pid = fork ();
  if (pid < 0)
    {
      fprintf(stdout, "Can not fork\n");
      return (1);
    }
  if (pid == 0)
    {
      fprintf(stdout, "New process\n");
    }
  else
    {
      fprintf(stdout, "Forked child %d\n", pid);
    }
  return (0);
}

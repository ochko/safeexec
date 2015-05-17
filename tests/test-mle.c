#include <stdlib.h>
#include <stdio.h>

int main (void)
{
  char *p;
  int k;
  int cnt;

  cnt = 0;
  for (;;)
    {
      p = (char *) malloc (1024);
      for (k = 0; k < 1024; k ++)
        p [k] = rand () % 256;
      cnt++;
      fprintf(stdout, "%d kb allocated\n", cnt);
    }
  return (0);
}

#include <stdlib.h>

int main (void)
{
  char *p;
  int k;
  for (;;)
    {
      p = (char *) malloc (1024);
      for (k = 0; k < 1024; k ++)
        p [k] = rand () % 256;
    }
  return (0);
}

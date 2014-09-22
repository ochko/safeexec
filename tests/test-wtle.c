#define _XOPEN_SOURCE 500
#include <unistd.h>

int main (void)
{
  for (;;)
    usleep (100);
  return (0);
}

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "error.h"

void error (char *format, ...)
{
  va_list p;
  if (format == NULL)
    error ("%s", strerror (errno));
  else
    {
      fprintf (stderr, "error: ");
      va_start (p, format);
      vfprintf (stderr, format, p);
      va_end (p);
      fprintf (stderr, "\n");
    }
  exit (EXIT_FAILURE);
}

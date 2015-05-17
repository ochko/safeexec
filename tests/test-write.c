#include <stdio.h>
#include <stdlib.h>

int main(void)
{
  FILE *fp;
  int i;

  fp = fopen("tests/file.dat", "w");
  if (fp == NULL) {
    perror("Error while opening the file");
    exit(EXIT_FAILURE);
  }

  for (i=0; i<=10; ++i)
    fprintf(fp, "%d, %d\n", i, i*i);

  fclose(fp);

  printf("Oops! Wrote file");

  return 0;
}

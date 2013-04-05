#include <stdio.h>
#include <stdlib.h>
   
int main(void)
{
  FILE *fp;
  int i;
   
  /* open the file */
  fp = fopen("file.dat", "w");
  if (fp == NULL) {
    printf("couldn't open file.dat for writing.\n");
    exit(1);
  }
   
  /* write to the file */
  for (i=0; i<=10; ++i)
    fprintf(fp, "%d, %d\n", i, i*i);
   
  /* close the file */
  fclose(fp);
   
  return 0;
}

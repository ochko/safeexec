#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
  FILE *fp;
  char ch;

  fp = fopen("tests/secret.txt" ,"r");
  if( fp == NULL ) {
    perror("Error while opening the file");
    exit(EXIT_FAILURE);
  }

  printf("Oops! This file ");

  while( ( ch = fgetc(fp) ) != EOF )
    printf("%c",ch);

  fclose(fp);

  return 0;
}

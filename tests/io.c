#include <stdio.h>

int main(void)
{
  int val;
  char name[256];
   
  printf( "Enter your age and name.\n" );
  scanf( "%d %s", &val, name ); 
  printf( "Your name is: %s -- and your age is: %d\n", name, val );
  return 0;
}

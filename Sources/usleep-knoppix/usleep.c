#include <stdio.h>

int main(int argc, char **argv)
 {
  unsigned long ut;
  if(argc<2||(ut=atol(argv[1]))<=0)
   {
    write(2,"Usage: usleep useconds\n", 23);
    return 1;
   }
  else usleep(ut);
  return 0;
 }

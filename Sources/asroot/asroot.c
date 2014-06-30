#if 0
 diet gcc -O2 -Wall -s -o asroot asroot.c
 sudo chown root asroot; sudo chmod 4755 asroot
 exit $?
#endif

/*************************************************************************\
* asroot - reimplementation of sudo with no checks, for Knoppix           *
* Circumvent a bug in sudo (dead syslog socket) that causes sudo to hang  *
* indefinitely                                                            *
* (C) Klaus Knopper 2013                                                  *
* License: GPL V2                                                         *
* This program has to be installed suid: install -m 4755 asroot /usr/bin/ *
\*************************************************************************/

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char **argv){

 if(argc < 2){
  write(2, "Usage: asroot command options...\n", 33);
  return -1;
 }

 if (!setuid(0) && !setgid(0)){
  char **arg = malloc(argc * sizeof(char*));
  int i;
  for(i=0; i<argc-1; i++) arg[i] = argv[i+1];
  return execvp(arg[0], arg);
 }

 write(1,"\"asroot\" needs to be installed chmod 4755.\n",44);
 return -1;
}

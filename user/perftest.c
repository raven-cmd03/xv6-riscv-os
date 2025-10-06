#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  uint64 cycles, time, instret;
  
  printf("Testing performance system calls:\n");
  
  cycles = rdcycle();
  time = rdtime();
  instret = rdinstret();
  
  printf("CPU Cycles: %ld\n", cycles);
  printf("Time: %ld\n", time);
  printf("Instructions Retired: %ld\n", instret);
  
  exit(0);
}

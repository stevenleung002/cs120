#include <stdio.h>
#include "aux.h"
#include "umix.h"

void Main()
{
  int pid = 0;
  pid = Yield(Getpid());
  Printf("result %d\n", pid);
} 

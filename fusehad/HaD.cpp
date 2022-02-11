#include <stdio.h>

#include "HaDFS.h"

int main(int argc, char *argv[])
{

  HaDFS fs;
  if (fs.userinit())
    {
      fprintf(stderr,"Can't fetch feed\n");
      return 99;
    };
  int status;
  status= fs.run(argc, argv);
  

  return status;
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv)
{

  printf("waiting %ds\n", atoi(argv[1]));
  sleep(atoi(argv[1]));
  return 0;
}
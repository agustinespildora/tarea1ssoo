#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv)
{
  printf("%d\n", atoi(argv[1]) + atoi(argv[2]));
  return 0;
}
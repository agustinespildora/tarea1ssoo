#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv)
{
  printf("waiting SUM 5 seg");
  sleep(5);
  printf("%d\n", atoi(argv[1]) + atoi(argv[2]));
  return 7;
}
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include "../file_manager/manager.h"

#define num 255
int arr[num];

char* path;
int n_process;
int counter = 0;

void alarm_handler(int sig)
{
  for (int i = 0; i <  counter; i++) {
    printf("printeando array, posición %d, valor %d, \n",i, arr[i]);
  }
  for (int i = 0; i < counter; i++) {
      kill(arr[i], SIGABRT);
  }
  printf("Uniendo archivos\n");
  exit(0);
  //kill(childA, SIGABRT);
  //kill(childB, SIGABRT); 
    
}
void abort_handler_worker(int sig)
{
    printf("Closing file");
    printf("Here's my PID: %d\n\n", getpid());
    exit(0);
    
}

void abort_handler_manager(int sig)
{
  for (int i = 0; i <  counter; i++) {
    printf("printeando array, posición %d, valor %d, \n",i, arr[i]);
  }
  for (int i = 0; i < counter; i++) {
        kill(arr[i], SIGABRT);
  }
  printf("Uniendo archivos");
  printf("Here's my PID: %d\n\n", getpid());
  exit(0);
    
}

void int_handler(int sig)
{
    printf("\n***IGNORING***\n");
    
}

// Funcion que ocupan los manager o root para escribir sus archivos
void output_rewrite_lines(char* process, char* child_process){

  char* child_path = calloc(4, sizeof(char));
  sprintf(child_path, "./%s.txt", child_process);
  FILE* child_file = fopen(child_path, "r");

  char* father_path = calloc(4, sizeof(char));
  sprintf(father_path, "./%s.txt", process);
  FILE* father_file = fopen(father_path, "a");

  char buffer[BUFFER_SIZE];
  while (fgets(buffer, BUFFER_SIZE, child_file)){
    fputs(buffer, father_file);
  }
  free(child_path);
  free(father_path);
  fclose(father_file);
  fclose(child_file);
}

char* concat_array(int n_args, char** array){
  char* line = calloc(BUFFER_SIZE, sizeof(char));
  for (int i = 0; i < n_args; i++)
  {
    strcat(line, array[i]);
    if (i < n_args - 1){
      strcat(line, ",");
    }
  }
  return line;
}

void output_worker(char* process, int n_args, char** args) {

  char* path = calloc(1, sizeof(char));
  sprintf(path, "./%s.txt", process);
  FILE* file = fopen(path, "a");

  char* line = concat_array(n_args, args);
  printf("Line: %s\n", line);
  fputs(line, file);
  free(line);
  fclose(file);
}

int main(int argc, char **argv){

  char* input_path = argv[1];
  InputFile* input = read_file(input_path);
  int process = atoi(argv[2]);
  printf("Here's my PID: %d\nHeres my line:%d\n", getpid(), process);
  path = input_path;
  n_process = process;
  // if (!argv[3])
  // {
  //   int timeout_father = 999999;
  // }
  // else {
  //   int timeout_father = atoi(argv[3]);
  // }
  
  char** line = input->lines[process];
  char* id = line[0];
  char* M = "M";
  char* R = "R";
  char* W = "W";

  //Si el identificador es R (Root) se hace lo mismo:
  if (strcmp(id, R) == 0){
    printf("Entra a Root, linea:%d\n", process);
    int timeout_son = atoi(line[1]);
    int n_childs = atoi(line[2]);

    signal(SIGALRM,(void (*)(int))alarm_handler); // Register signal handler

    for (int i = 3; i < 3 + n_childs; i++){

      char* line_to_exec = line[i];

      int status;
      pid_t child_pid = fork();

      //En caso de que childpid < 0 algun error hubo
      if (child_pid >= 0){
        //Hijo
        if (child_pid == 0){
          // execlp(ejecutable, argv[0] (ejecutable de nuevo), argv[1], ...)
          printf("Root: Entra al hijooo\n");
          execlp("./crtree", "./crtree", input_path, line_to_exec, (char*)NULL);
        }
        //Padre
        else{
          arr[counter] = child_pid;
          counter++;
          alarm(timeout_son);
          wait(&status); //Aquí agregar el deadline del timeout_father de alguna forma, y si se pasa, mandar SIGABRT
          int exit_code =  WEXITSTATUS(status);
          printf("Process %d finished!\n", child_pid);
          printf("Exit code: %d\n", exit_code);
          // char* process_line = argv[2];
          // output_rewrite_lines(process_line, line_to_exec);
        }
      }
      else{
        perror("fork");
        exit(-1);
        return -1;
      }
    }
    return 0;
  }

  else if (strcmp(id, M) == 0 ){
    printf("Entra a Manager, linea:%d\n", process);
    int timeout_son = atoi(line[1]);
    int n_childs = atoi(line[2]);
    signal(SIGABRT,(void (*)(int))abort_handler_manager); // Register signal handler
    signal(SIGALRM,(void (*)(int))alarm_handler); // Register signal handler

    for (int i = 3; i < 3 + n_childs; i++){

      char* line_to_exec = line[i];

      int status;
      pid_t child_pid = fork();

      //En caso de que childpid < 0 algun error hubo
      if (child_pid >= 0){
        //Hijo
        if (child_pid == 0){
          // execlp(ejecutable, argv[0] (ejecutable de nuevo), argv[1], ...)
          printf("Manager: Entra al hijooo\n");
          execlp("./crtree", "./crtree", input_path, line_to_exec, (char*)NULL);
        }
        //Padre
        else{
          printf("Soy proceso %d\nMi hijo es%d\n", process, child_pid);
          arr[counter] = child_pid;
          printf("arr[counter] %d\ncounter%d\n", arr[counter], counter);
          counter++;
          alarm(10);
          wait(&status); //Aquí agregar el deadline del timeout_father de alguna forma, y si se pasa, mandar SIGABRT
          int exit_code =  WEXITSTATUS(status);
          printf("Process %d finished!\n", child_pid);
          printf("Exit code: %d\n", exit_code);
          // char* process_line = argv[2];
          // output_rewrite_lines(process_line, line_to_exec);
        }
      }
      else{
        perror("fork");
        exit(-1);
        return -1;
      }
    }
    return 0;
  
  }

  // Si el identificador es W (Worker):

  else if (strcmp(id, W) == 0 ){
    printf("Entra a Worker, linea:%d\n", process);

    //Obtenemos el nombre del ejecutable (lo guardamos en exe) y los argumentos asociados (args)
    char* exe = line[1];
    printf("Ejecutable de worker:%s\n", exe);
    int n_args = atoi(line[2]);
    //CAMBIO n_args + 1 -> n_args + 2
    char** args = calloc(n_args + 5, sizeof(char*));
    args[0] = exe;
    for (int j = 0; j < n_args; j++){
      args[j + 1] = line[3 + j];
    }
    args[n_args + 1] = (char*)NULL;
    

    signal(SIGABRT,(void (*)(int))abort_handler_worker); // Register signal handler
    //Creamos el hijo
    int status;
    pid_t childpid = fork();

    //En caso de que childpid < 0 algun error hubo
    if (childpid >= 0){
      //Hijo
      if (childpid == 0){

        execvp(exe, args);
    
      }
      //Padre
      else{
        wait(&status); //Aquí agregar el deadline del timeout_father de alguna forma, y si se pasa, mandar SIGABRT
        char* exit_code;
        sprintf(exit_code, "%d", WEXITSTATUS(status));
        ////////////////////////////////////////////////////////////////
        //PENDING: OBTENER TIEMPO DE EJECUCION Y SI FUE INTERRUMPIDO O NO
        char* time = "0";
        char* interrupted = "0";
        ///////////////////////////////////////////////////////////////
        args[n_args + 2] = time;
        args[n_args + 3] = exit_code;
        args[n_args + 4] = interrupted;
        // output_worker(argv[2], n_args, args);
        exit(0);

      }
    }
    else{
      perror("fork");
      exit(-1);
      return -1;
    }
    input_file_destroy(input);
    free(args);
    return 0;
  }
  else{
    printf("Error: no entra ni a manager ni a worker\n");
  }
}



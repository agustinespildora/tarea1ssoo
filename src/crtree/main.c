#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include "../file_manager/manager.h"

#define num 255
int arr[num];

char* path;
int n_lines;
int n_process;
int counter = 0;
char time_taken[10];
char exit_code[2];
char interrupted[3];

// char* time_taken;
// char* exit_code;
// char* interrupted;

void alarm_handler(int sig)
{
  for (int i = 0; i <  counter; i++) {
    printf("printeando array, posición %d, valor %d, \n",i, arr[i]);
  }
  for (int i = 0; i < counter; i++) {
      kill(arr[i], SIGABRT);
  }
  int wpid;
  int status_handler;
  while ((wpid = wait(&status_handler)) > 0);
  printf("mandando señal a los hijos\n");
  exit(0);    
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
  int wpid;
  int status_handler;
  while ((wpid = wait(&status_handler)) > 0);
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

  char child_path[15];
  sprintf(child_path, "./%s.txt", child_process);
  FILE* child_file = fopen(child_path, "r");
  char father_path[7];
  sprintf(father_path, "./%s.txt", process);
  FILE* father_file = fopen(father_path, "a");
  char buffer[BUFFER_SIZE];
  while (fgets(buffer, BUFFER_SIZE, child_file)){
    printf("Linea de archivo del proceso %s: %s\n", child_process, buffer);
    fputs(buffer, father_file);
  }
  fclose(father_file);
  fclose(child_file);
}

char* concat_array(int n_args, char** array){
  char* line = calloc(BUFFER_SIZE, sizeof(char));
  for (int i = 0; i < n_args + 4; i++)
  {
    printf("concat arrays array[%i]: %s\n", i, array[i]);
    strcat(line, array[i]);
    if (i < n_args + 3){
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
  printf("Line de worker: %s\n", line);
  fputs(line, file);
  free(line);
  fclose(file);
}

int main(int argc, char **argv){

  char* input_path = argv[1];
  InputFile* input = read_file(input_path);
  n_lines = input->len;
  char* process_str = argv[2];
  int process = atoi(process_str);
  printf("Here's my PID: %d\nHeres my line:%d\n", getpid(), process);
  path = input_path;
  n_process = process;
  
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
    char* line_id_arr[n_childs];
    int status;
    for (int i = 0; i < n_childs; i++){

      int line_int = atoi(line[i + 3]);
      char line_to_exec[2];
      sprintf(line_to_exec, "%i", line_int);
      line_id_arr[i] = line_to_exec;
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
          signal(SIGINT,(void (*)(int))alarm_handler); // Register signal handler
          arr[counter] = child_pid;
          counter++;
        }
      }
      else{
        perror("fork");
        exit(-1);
        return -1;
      }
    }
    /////////////////////////////////////////////////////////////
    alarm(15);
    ////////////////////////////////////////////////////////////////
    int wpid;
    while ((wpid = wait(&status)) > 0){
      printf("Soy root, waiting son (while):%d\n", wpid);
    };
    printf("Soy root salí del while wpid wait\n");
    for (int j = 0; j < counter; j++)
    {
      output_rewrite_lines(argv[2], line_id_arr[j]);
    }
    input_file_destroy(input);
    return 0;
  }

  else if (strcmp(id, M) == 0 ){
    printf("Entra a Manager, linea:%d\n", process);
    int timeout_son = atoi(line[1]);
    int n_childs = atoi(line[2]);
    signal(SIGINT,(void (*)(int))int_handler); // Register signal handler
    signal(SIGABRT,(void (*)(int))abort_handler_manager); // Register signal handler
    signal(SIGALRM,(void (*)(int))alarm_handler); // Register signal handler

    char* line_id_arr[n_childs];
    int status;
    for (int i = 0; i < n_childs; i++){
      int line_int = atoi(line[i + 3]);
      char line_to_exec[2];
      sprintf(line_to_exec, "%i", line_int);
      line_id_arr[i] = line_to_exec;

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
          
        }
      }
      else{
        perror("fork");
        exit(-1);
        return -1;
      }
    }
    //////////////////////////////////////////////////////////
    alarm(15);
    //////////////////////////////////////////////////////////
    int wpid;
    while ((wpid = wait(&status)) > 0){
      printf("Soy manager, waiting son (while):%d\n", wpid);
    };
    printf("Soy manager salí del while wpid wait\n");
    for (int j = 0; j < counter; j++)
    {
      output_rewrite_lines(argv[2], line_id_arr[j]);
    }
    input_file_destroy(input);
    return 0;
  
  }

  // Si el identificador es W (Worker):

  else if (strcmp(id, W) == 0 ){
    printf("Entra a Worker, linea:%d\n", process);
    signal(SIGINT,(void (*)(int))int_handler); // Register signal handler
    char* exe = line[1];
    printf("Ejecutable de worker:%s\n", exe);
    int n_args = atoi(line[2]);
    char** args = calloc(n_args + 2, sizeof(char*));
    char** args_to_file = calloc(n_args + 4, sizeof(char*));

    args[0] = exe;
    args_to_file[0] = exe;
    for (int j = 1; j < n_args + 1; j++){
      int line_int = atoi(line[j + 3]);
      
      char arg[2];
      sprintf(arg, "%i", line_int);
      printf("ARGUMENTOS WORKER: %s n_args %i\n", arg, n_args);
      args[j] = arg;
      args_to_file[j] = arg;
    }
    printf("sale del for de argumentos worker\n");
    args[n_args + 1] = (char*)NULL;
    
    
    signal(SIGABRT,(void (*)(int))abort_handler_worker); // Register signal handler
    //Creamos el hijo
    int status;
    int signaled;
    time_t start, end;
    time(&start);
    pid_t childpid = fork();
    //En caso de que childpid < 0 algun error hubo
    if (childpid >= 0){
      //Hijo
      if (childpid == 0){

        execvp(exe, args);
    
      }
      //Padre
      else{
        printf("LLEGA A ANTES DEL WAIT DE WORKER\n");
        wait(&status);
        printf("LLEGA A DESPUES DEL WAIT DE WORKER\n");
        time(&end);
        int exe_time = ((double) (end - start));
        sprintf(interrupted, "%d", WIFSIGNALED(signaled));
        sprintf(exit_code, "%d", WEXITSTATUS(status));
        sprintf(time_taken, "%d", exe_time);
        printf("Llega a despues de los SPRINTF\n");
        args_to_file[n_args + 1] = time_taken;
        args_to_file[n_args + 2] = exit_code;
        args_to_file[n_args + 3] = interrupted;
        char process_f[2];
        sprintf(process_f, "%i", process);
        printf("LLEGA AL FINAL DEL WORKER, ultimo arg:%s time: %s, exit: %s, interrupted:%s\n", args_to_file[n_args], args_to_file[n_args + 1], args_to_file[n_args + 2], args_to_file[n_args + 3]);
        for (int r = 0; r < n_args + 4; r++)
        {
          printf("BUG BUG BUG args_to_file[%i]:%s\n", r, args_to_file[r]);
        }
        
        output_worker(process_f, n_args, args_to_file);
        free(args);
        free(args_to_file);
        input_file_destroy(input);
        return 0;
      }
    }
    else{
      perror("fork");
      exit(-1);
      return -1;
    }
    return 0;
  }
  else{
    printf("Error: no entra ni a manager ni a worker\n");
  }
}



#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <ctype.h>
#include "../file_manager/manager.h"

#define num 255
int arr[num];

int abort_flag = 0;
int alarm_flag = 0;
int sigint_flag = 0;

char* path;
int n_lines;
int counter = 0;
char time_taken[10];
char exit_code[10];
char interrupted[10];

void alarm_handler(int sig)
{
  alarm_flag = 1;  
}

void sigint_handler(int sig)
{
  sigint_flag = 1;  
}

void abort_handler_manager(int sig)
{
  for (int i = 0; i <  counter; i++) {
    printf("posición %d, valor %d, \n",i, arr[i]);
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

void abort_handler(int sig){
  abort_flag = 1;
}

// Funcion que ocupan los manager o root para escribir sus archivos
void output_rewrite_lines(int process, int child_process){

  char child_path[20];
  sprintf(child_path, "./%i.txt", child_process);
  FILE* child_file = fopen(child_path, "r");
  char father_path[20];
  sprintf(father_path, "./%i.txt", process);
  FILE* father_file = fopen(father_path, "a");
  char buffer[BUFFER_SIZE];
  printf("Manager o Root de la linea %i: Reescribiendo lineas de %s en %s\n", process, child_path, father_path);
  while (fgets(buffer, BUFFER_SIZE, child_file)){
    // Le agregué esta linea para quitarle el /n si es que tenía pero no se resolvió el problema de escritura.
    printf("Pasando linea: %s del archivo %s al %s\n", buffer, child_path, father_path);
    fputs(buffer, father_file);
  }
  fclose(father_file);
  fclose(child_file);
}

char* concat_array(int n_args, char** array){
  char* line = calloc(BUFFER_SIZE, sizeof(char));
  for (int i = 0; i < n_args + 4; i++)
  {
    printf("Concatenando args a una linea -> array[%i] = %s\n", i, array[i]);
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
  printf("Worker de la linea %s: Esribiendo linea: %s en ./%s.txt\n", process, line, process);
  fputs(line, file);
  free(line);
  free(path);
  fclose(file);
}

//FUNCION OBTENIDA DE GITHUBGIST https://gist.github.com/Morse-Code/5310046
char* stringRemoveNonAlphaNum(char* str)
{
  unsigned long i = 0;
  unsigned long j = 0;
  char c;

  while ((c = str[i++]) != '\0')
  {
      if (isalnum(c))
      {
          str[j++] = c;
      }
  }
  str[j] = '\0';
  return str;
}

int main(int argc, char **argv){

  char* input_path = argv[1];
  InputFile* input = read_file(input_path);
  n_lines = input->len;
  char* process_str = argv[2];
  int process = atoi(process_str);
  path = input_path;
  
  char** line = input->lines[process];
  char* id = line[0];
  char* M = "M";
  char* R = "R";
  char* W = "W";

  //Si el identificador es R (Root) se hace lo mismo:
  if (strcmp(id, R) == 0){
    printf("Linea %d es Root y su PID es: %d\n", process, getpid());
    int timeout_son = atoi(line[1]);
    int n_childs = atoi(line[2]);
    int child_array[n_childs];

    
    int child_process_arr[n_childs];
    int status;

    for (int i = 0; i < n_childs; i++){
      int line_int = atoi(line[i + 3]);
      char line_to_exec[2];
      sprintf(line_to_exec, "%d", line_int);
      child_process_arr[i] = atoi(line[i + 3]);
      pid_t child_pid = fork();

      //En caso de que childpid < 0 algun error hubo
      if (child_pid >= 0){
        //Hijo
        if (child_pid == 0){
          // execlp(ejecutable, argv[0] (ejecutable de nuevo), argv[1], ...)
          printf("Root: Voy a ejecutar mi hijo que está en la linea %d\n", line_int);
          execlp("./crtree", "./crtree", input_path, line_to_exec, (char*)NULL);
        }
        //Padre
        else{
          signal(SIGINT,(void (*)(int))sigint_handler); 
          signal(SIGALRM,(void (*)(int))alarm_handler); 
          // arr[counter] = child_pid;
          child_array[counter] = child_pid;
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
    alarm(timeout_son);
    ////////////////////////////////////////////////////////////////
    
    while (true) {
      // Espera a cualquier hijo (por eso el parametro -1, cuando alguno termine debería retornar su pid)
      pid_t done = waitpid(-1, &status, WNOHANG);      
      // Cuando retorna -1 es porque no quedan hijos ejecutando
      if (done == -1) {
          if (errno == ECHILD) break; // no more child processes
      }
      // Si se detecta la flag de alarm (timeout)
      if (alarm_flag || sigint_flag){
        printf("ALARMA: %i o SIGINT: %i EN ROOT", alarm_flag, sigint_flag);
        // Manda señal a sus hijos
        for (int i = 0; i < n_childs; i++) {
          printf("posición %d, valor %d, \n",i, child_array[i]);
          kill(child_array[i], SIGABRT);
        }
        // Espera a que todos terminen
        while (true) {
          int status;
          pid_t done2 = waitpid(-1, &status, WNOHANG);      
          if (done2 == -1) {
            printf("TERMINARON MIS HIJOS");
            if (errno == ECHILD) break; // no more child processes
          }
        }
        printf("Uniendo archivos");
        printf("Here's my PID: %d\n\n", getpid());
        alarm_flag = 0;
        // Ejecuta la escritura
        for (int j = 0; j < n_childs; j++)
        {
          output_rewrite_lines(process, child_process_arr[j]);
        }
        input_file_destroy(input);
        return 0;
      }
      if (sigint_flag == 1){
        sigint_flag = 0;
        for (int i = 0; i < n_childs; i++) {
          printf("posición %d, valor %d, \n",i, child_array[i]);
          kill(child_array[i], SIGABRT);
        }
        printf("Uniendo archivos");
        printf("Here's my PID: %d\n\n", getpid());
      }
      
    }
    
    // Si los hijos terminaron de ejecutar normalmente, sale del while grande y escribe los archivos
    for (int j = 0; j < n_childs; j++)
    {
      output_rewrite_lines(process, child_process_arr[j]);
    }
    input_file_destroy(input);
    return 0;
  }

  else if (strcmp(id, M) == 0 ){
    printf("Linea %d es Manager y su PID es: %d\n", process, getpid());
    int timeout_son = atoi(line[1]);
    int n_childs = atoi(line[2]);
    signal(SIGINT,(void (*)(int))sigint_handler); 
    signal(SIGABRT,(void (*)(int))abort_handler); 
    signal(SIGALRM,(void (*)(int))alarm_handler); 

    int child_process_arr[n_childs];
    int status;
    int child_array[n_childs];
    for (int i = 0; i < n_childs; i++){
      int line_int = atoi(line[i + 3]);
      char line_to_exec[2];
      sprintf(line_to_exec, "%d", line_int);
      child_process_arr[i] = line_int;

      pid_t child_pid = fork();
      //En caso de que childpid < 0 algun error hubo
      if (child_pid >= 0){
        //Hijo
        if (child_pid == 0){
          // execlp(ejecutable, argv[0] (ejecutable de nuevo), argv[1], ...)
          printf("Manager de linea %d: voy a ejecutar mi hijo que está en la linea %d\n", process, line_int);
          execlp("./crtree", "./crtree", input_path, line_to_exec, (char*)NULL);
        }
        //Padre
        else{
          child_array[counter] = child_pid;
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
    alarm(timeout_son);
    //////////////////////////////////////////////////////////
    // Misma funcionalidad que el while de ROOT
    while (true) {
      pid_t done = waitpid(-1, &status, WNOHANG);      
      if (done == -1) {
          if (errno == ECHILD) break; // no more child processes
      }
      // Si se activa la alarma o recibe señal de aborto de root actúa igual:
      if (abort_flag == 1 || alarm_flag == 1){
        printf("ALARMA: %i O ABORT: %i EN MANAGER\n", alarm_flag, abort_flag);
        for (int i = 0; i < n_childs; i++) {
          printf("posición %d, valor %d, \n",i, child_array[i]);
          kill(child_array[i], SIGABRT);
        }
        while (true){
          pid_t done2 = waitpid(-1, &status, WNOHANG);      
          if (done2 == -1) {
          printf("\nMANAGER:TERMINARON MIS HIJOS\n");
          if (errno == ECHILD) break; // no more child processes
          }
        }
        
        printf("MANAGER Uniendo archivos\n");
        printf("Here's my PID: %d\n\n", getpid());
        abort_flag = 0;
        alarm_flag = 0;
        
        // UNIR Y ESCRIBIR ARCHIVOS
        for (int j = 0; j < counter; j++)
        {
          output_rewrite_lines(process, child_process_arr[j]);
        }
        input_file_destroy(input);
        return 0;
      }
      if (sigint_flag == 1){
        sigint_flag = 0;
        printf("IGNORING");
      }
    }

    for (int j = 0; j < counter; j++)
    {
      output_rewrite_lines(process, child_process_arr[j]);
    }
    input_file_destroy(input);
    return 0;
  
  }

  // Si el identificador es W (Worker):

  else if (strcmp(id, W) == 0 ){
    printf("Linea %d es Worker y su PID es: %d\n", process, getpid());

    signal(SIGINT,(void (*)(int))sigint_handler);
    signal(SIGABRT,(void (*)(int))abort_handler);

    char* exe = line[1];
    int n_args = atoi(line[2]);
    char** args = calloc(n_args + 2, sizeof(char*));
    char** args_to_file = calloc(n_args + 4, sizeof(char*));
    //exe[strcspn(exe, "\n")] = 0; Esta linea debería quitar el salto de linea a exe, pero este no es el problema

    args[0] = exe;
    args_to_file[0] = exe;
    for (int j = 1; j < n_args + 1; j++){

      //AQUI VAN UNA SERIE DE INTENTOS PARA SACAR EL SALTO DE LINEA MIESTERIOSO

      //INTENTO 5 EUREKA!
      args[j] = stringRemoveNonAlphaNum(line[j + 2]);
      args_to_file[j] = stringRemoveNonAlphaNum(line[j + 2]);

    }
    args[n_args + 1] = (char*)NULL;
    printf("Worker de la linea %d: voy a ejecutar %s\n", process, exe);
     
    //Creamos el hijo
    int status;
    time_t start, end;
    time(&start);
    pid_t childpid = fork();
    //En caso de que childpid < 0 algun error hubo
    if (childpid >= 0){
      //Hijo
      if (childpid == 0){
        signal(SIGINT,(void (*)(int))sigint_handler);
        execvp(exe, args);
    
      }
      //Padre
      else{
        // Misma funcionalidad que root y manager
        while (true) {
          pid_t done = waitpid(-1, &status, WNOHANG);      
          if (done == -1) {
              if (errno == ECHILD) break; // no more child processes
          }
          // Si se recibe señal de aborto, escribe el programa
          if (abort_flag == 1){
            // Escribir archivo
            printf("ABORTANDO WORKER Y CERRANDO ARCHIVO\n");
            time(&end);
            int exe_time = ((double) (end - start));
            sprintf(interrupted, "%d\n", abort_flag);
            sprintf(exit_code, "%d", WEXITSTATUS(status));
            sprintf(time_taken, "%d", exe_time);
            args_to_file[n_args + 1] = time_taken;
            args_to_file[n_args + 2] = exit_code;
            args_to_file[n_args + 3] = interrupted;
            char process_f[2];
            sprintf(process_f, "%i", process);

            printf("Worker de la linea %d (%s): INTERRUMPIDO!! Ahora voy a escribir mi archivo\n", process, exe);
            output_worker(process_f, n_args, args_to_file);
            free(args);
            free(args_to_file);
            input_file_destroy(input);
            abort_flag = 0;
            return 0;
            
          }
          if (sigint_flag ==1){
            sigint_flag = 0;
            printf("IGNORING\n");
            printf("SOY WORKER, ESPERO QUE MI HIJO TERMINE\n");

          }

        }
        
        time(&end);
        int exe_time = ((double) (end - start));
        printf("EXETIME:%d\n", exe_time);
        sprintf(interrupted, "%d\n", abort_flag);
        sprintf(exit_code, "%d", WEXITSTATUS(status));
        sprintf(time_taken, "%d", exe_time);
        args_to_file[n_args + 1] = time_taken;
        args_to_file[n_args + 2] = exit_code;
        args_to_file[n_args + 3] = interrupted;
        char process_f[2];
        sprintf(process_f, "%i", process);

        printf("Worker de la linea %d (%s): Terminé!! Ahora voy a escribir mi archivo\n", process, exe);
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



#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// forward declarations
#define KRED "\x1B[31m"
#define KNRM "\x1B[0m"
#define KYEL "\x1B[33m"
void redirect_forward();   // >
void redirect_backward();  // <
void get_processes();             // if processes cmd
void cd();                        // basic cd
void execute_normal_process();    // run normal process
void execute_bg_process();        // run bg process
int process_counter = 0;
static int bg_process_table[1024] = {};
static char bg_cmds_table[32][128];  // up to 32 background cmds
static char user_command[1024];
static char *command_token;
static char dup_input[1024];

int main() {
  char cwd[1024];
  char hostname[128];
  char username[128];
  gethostname(hostname, 128);
  getlogin_r(username, 128);
  int background = 0;  // flag or split process fx, 1 if bg
  printf(
      "TCSH - A small UNIX shell written in C.\n\n\"bg cmd\" to run a process "
      "in background, EX: bg sleep 5\n\"processes\" to view zombie "
      "threads\n\"exit\" command exits the shell\n\"< >\" redirection "
      "supported\n\n");
  char *carrot_index;
  for (;;) {
    getcwd(cwd, sizeof(cwd));
    printf("%s%s%s@%s%s%s$ ", KRED, username, KNRM, KYEL, cwd, KNRM);
    // get user input, duplicate, and tokenize
    fgets(user_command, 1024, stdin);
    strcpy(dup_input, user_command);
    command_token = strtok(user_command, " )(><&\t\n\r");
    if (!command_token) {
      continue;  // try again fgets for user input
    }            // now given command, check against posssible values:
    else if (strcmp(command_token, "exit") == 0) {
      exit(0);  // exit return code 0 for no errors
    } else if (strcmp(command_token, "processes") == 0) {
      get_processes();
    } else if (strcmp(command_token, "cd") == 0) {
      cd();
    } else if ((carrot_index = strchr(dup_input, '>'))) {
      redirect_forward();
    } else if ((carrot_index = strchr(dup_input, '<'))) {
      redirect_backward();
    }
    // next determine regular or background process
    else {
      if (strcmp(command_token, "bg") == 0) {
        command_token = strtok(NULL, " \t\n()<>|&;");
        execute_bg_process();
      } else {
        execute_normal_process();
      }
    }
  }
  return 0;
}

void get_processes() {
  if (process_counter > 0) {
    printf("PID:    Command: \n");
    for (int i = 0; i < process_counter; i++) {
      printf("%d    ", bg_process_table[i]);
      printf("%s\n", bg_cmds_table[i]);
    }
  } else {
    printf("No background processes.\n");
  }
}

void cd() {
  char *home_path;  // home path is set to return val of getenv("HOME")
  home_path = getenv("HOME");
  command_token = strtok(NULL, " )(><&\t\n\r");
  // two cases: cd or cd path
  // case: "cd"
  if (command_token == NULL) {
    if (chdir(home_path) == -1) {
      perror("Error");
    }
    return;
  }
  // case: "cd path"
  if (chdir(command_token) == -1) {
    perror("Error");
  }
}

void redirect_forward() {
  char env[64] = "/bin/";
  int status = 0;
  char *args[1024];
  int arg_counter = 0;
  FILE *file_pointer;
  pid_t pid;
  // fork
  pid = fork();
  if (pid == -1) {  // fork error
    perror("Error");
    exit(EXIT_FAILURE);
  }
  // child
  else if (pid == 0) {
    args[arg_counter] = command_token;
    while (command_token != NULL) {
      command_token = strtok(NULL, " )(><&\t\n\r");
      arg_counter++;
      args[arg_counter] = command_token;
    }
    file_pointer = freopen(args[arg_counter - 1], "w", stdout);
    if (file_pointer == NULL) {
      perror("Error");
      return;
    }
    args[arg_counter - 1] = NULL;
    strcat(env, args[0]);
    if (execve(env, args, NULL) == -1) {
      perror("Error");
    }
    fclose(file_pointer);
    _exit(0);
  }
  // parent
  else {
    waitpid(pid, &status, 0);

    if (WIFEXITED(status) == 0) {
      printf("Child didn't exit normally");
    }
  }
}

void redirect_backward() {
  char env[64] = "/bin/";
  int status = 0;
  char *args[1024];
  int arg_counter = 0;
  FILE *file_pointer;
  pid_t pid;
  // fork
  pid = fork();
  if (pid == -1) {
    perror("Error");
    exit(EXIT_FAILURE);
  }
  // child
  else if (pid == 0) {
    args[arg_counter] = command_token;
    while (command_token != NULL) {
      command_token = strtok(NULL, " )(><&\t\n\r");
      arg_counter++;
      args[arg_counter] = command_token;
    }
    printf("%s", args[arg_counter - 2]);
    // Second to last token should be file name, redirect output
    file_pointer = freopen(args[arg_counter - 2], "w", stdout);
    if (file_pointer == NULL) {
      perror("Error");
      return;
    }
    args[arg_counter - 1] = NULL;
    strcat(env, args[0]);
    if (execve(env, args, NULL) == -1) {
      perror("Error");
    }
    fclose(file_pointer);
    _exit(0);
  }
  // parent
  else {
    waitpid(pid, &status, 0);
    if (WIFEXITED(status) == 0) {
      printf("Child didn't exit normally");
    }
  }
}

void execute_normal_process() {
  char env[64] = "/bin/";
  int status = 0;
  char *args[1024];
  int arg_counter = 0;
  pid_t pid;
  // fork
  pid = fork();
  if (pid < 0) {
    perror("Error");
    exit(EXIT_FAILURE);
  }
  // child
  else if (pid == 0) {
    args[arg_counter] = command_token;
    while (command_token != NULL) {
      command_token = strtok(NULL, " )(><&\t\n\r");
      arg_counter++;
      args[arg_counter] = command_token;
    }
    strcat(env, args[0]);
    if (execve(env, args, NULL) == -1) {
      perror("Error");
    }
    _exit(0);
    // exit(1);
  }
  // parent
  else {
    waitpid(pid, &status, 0);
    // normal exit if status
    if (WIFEXITED(status) == 0) {
      printf("Child did not exit normally.");
    }
  }
}

void execute_bg_process() {
  char env[64] = "/bin/";
  int status = 0;
  char *args[1024];
  args[0] = command_token;
  int arg_counter = 0;
  int corpse;
  pid_t pid;
  // fork
  pid = fork();
  if (pid < 0) {
    perror("Error");
    exit(EXIT_FAILURE);
  }
  // parent
  if (pid != 0) {
    bg_process_table[process_counter] = pid;
    strcpy(bg_cmds_table[process_counter], args[0]);
    printf("Added Background Process: %d ", bg_process_table[process_counter]);
    printf("%s\n", bg_cmds_table[process_counter]);
    // printf("%d\n", pid);
    // printf("%d\n", process_counter);
    process_counter++;
  }
  // child
  else if (pid == 0) {
    args[arg_counter] = command_token;
    while (command_token != NULL) {
      command_token = strtok(NULL, " )(><&\t\n\r");
      arg_counter++;
      args[arg_counter] = command_token;
    }
    // close stdout and stderr to child of bg process
    fclose(stdout);
    fclose(stderr);
    strcat(env, args[0]);
    if (execve(env, args, NULL) == -1) {
      perror("Error");
    }
    _exit(0);
    // exit(1);
  }
  // parent
  waitpid(-1, &status, WNOHANG);
}
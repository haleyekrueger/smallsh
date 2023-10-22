// Author: Haley Krueger
// Citing: I used the following resources to research for this program: office hours via Teams, the Smallsh Discord channel, and Ed Discussions
// Date: Wed March 1 2023

#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h> // for waitpid()
#include <stdint.h>
#include <stddef.h>
#include <fcntl.h> // for open()

// search and replace function
// Author: Ryan Gambord via tutorial in Smallsh spec
char *str_gsub(char *restrict *restrict haystack, char const *restrict needle, char const *restrict sub) {
  char *str = *haystack;
  if (strcmp(needle, "~/") == 0) {  // special case to ensure that '/' is retained in the original string
    needle = "~";
  };

  size_t haystack_len = strlen(str);
  size_t const needle_len = strlen(needle), sub_len = strlen(sub);
  for (; (str = strstr(str, needle));) {
    ptrdiff_t off = str - *haystack;
    if (sub_len > needle_len) {
      str = realloc(*haystack, sizeof **haystack * (haystack_len + sub_len - needle_len + 1));
      if (!str) goto exit;
      *haystack = str;
      str = *haystack + off;}
    memmove(str + sub_len, str + needle_len, haystack_len + 1 - off - needle_len);
    memcpy(str, sub, sub_len);
    haystack_len = haystack_len + sub_len - needle_len;
    str += sub_len;
  }
  str = *haystack;
  if (sub_len < needle_len) {
    str = realloc(*haystack, sizeof **haystack * (haystack_len + 1));
    if (!str) goto exit;
    *haystack = str;}
exit:
  return str;}

  // signal handler
  void handle_SIGINT(int signo){
    return; // dummy handler does nothing
  }

// main
int main(void) {

  // sigaction structs
  struct sigaction SIGINT_action = {}, ignore_action = {}, default_action = {};

  ignore_action.sa_handler = SIG_IGN;   // ignore to be used on SIGINT

  sigaction(SIGTSTP, &ignore_action, NULL); // set SIGTSTP to ignore for entirety of smallsh

  SIGINT_action.sa_handler = handle_SIGINT;  // dummy handler returns SIGINT_action to fresh state
  sigfillset(&SIGINT_action.sa_mask);
  SIGINT_action.sa_flags = 0;

  sigaction(SIGINT, &ignore_action, &default_action); // set SIGINT to ignore for most of smallsh, store the default struct

  // initializing for use in getline inside for loop
  char *commandline = NULL;
  size_t len = 0;

  // declare global chars, assign default values where necessary
  pid_t bg_pid;
  pid_t pid = getpid();
  char *bg_pid_default = "";
  char *restrict bg_pid_ptr;
  bg_pid_ptr = bg_pid_default;
  int last_exit_int = 0;
  int bg_exit_int;

  // main smallsh loop
  for(;;) {

    pid_t finished_process;

    // check for un-waited-for background processes and print statements accordingly
    if ((finished_process = waitpid(0, &bg_exit_int, WNOHANG | WUNTRACED)) > 0) { 

      if (WIFEXITED(bg_exit_int)) {
        bg_exit_int = WEXITSTATUS(bg_exit_int);
        fprintf(stderr, "Child process %jd done. Exit status %d.\n", (intmax_t) bg_pid, bg_exit_int);
      }

      if (WIFSTOPPED(bg_exit_int)) {
        fprintf(stderr, "Child process %jd stopped. Continuing.\n", (intmax_t) bg_pid);
        kill(bg_pid, SIGCONT);
      }

      if (WIFSIGNALED(bg_exit_int)) {
        pid_t term_signal;
        term_signal = WTERMSIG(bg_exit_int);
        fprintf(stderr, "Child process %jd done. Signaled %d.\n", (intmax_t) bg_pid, term_signal);
      }

    }

    // get input
    FILE *input;
    ssize_t commandlinesize;

    sigaction(SIGINT, &SIGINT_action, &ignore_action); // set SIGINT to empty while getting line, save ignore struct

    errno = 0; // reset errno (not totally sure this made any difference)

    input = stdin;
    commandlinesize = getline(&commandline, &len, input);
    if (feof(stdin)) break;   // stop taking input once eof is hit
    if (strcmp(commandline, "\n") ==0) {
      goto end_of_loop;    // ignore blank lines and reloop
    }

    sigaction(SIGINT, &ignore_action, NULL); // set SIGINT to ignore until next loop 

    // prompt for input and get environment variables
    char *prompt = getenv("PS1");
    if (prompt == NULL) {
      prompt = "";
    }
    printf("%s", prompt);

    char *home_var = getenv("HOME");
    if (home_var == NULL) {
      home_var = "";
    }

    // tokenize commandline
    char *words[513] = {};
    int i = 0;
    char *delim = getenv("IFS");
    if (delim == NULL) {
      delim = " \t\n";
    }

    // define token pointers to be used in the string replacement function
    char *tilde = "~/";
    char *restrict smallsh_id_token = "$$";
    char *restrict last_fg_exitst_token = "$?";
    char *restrict rec_bg_proc_id_token = "$!";

    // put global variables into correct string format to be used in words[]
    char last_exit_str[8];
    sprintf(last_exit_str, "%d", last_exit_int); 
    char *restrict exit_status_ptr = last_exit_str; 

    char pid_str[8];
    sprintf(pid_str, "%d", pid);
    char *restrict pidptr = pid_str;

    char *token;
    token = strtok(commandline, delim);
    
    while (token) {
      words[i] = strdup(token);   // duplicate into words[] so that pointers don't get messed up
      char *new_string;
      
      // expansion functionality
      new_string = str_gsub(&words[i], smallsh_id_token, pidptr);
      words[i] = new_string;
      new_string = str_gsub(&words[i], last_fg_exitst_token, exit_status_ptr);
      words[i] = new_string;
      new_string = str_gsub(&words[i], rec_bg_proc_id_token, bg_pid_ptr);
      words[i] = new_string;

      // special search and replace behavior for home environment variable
      if (strncmp(words[i], tilde, 2) == 0) {
        new_string = str_gsub(&words[i], tilde, getenv("HOME"));
      } 
      words[i] = new_string;
      i++;
      token = strtok(NULL, delim);
    }

    // execute built-in functions without parsing
    if (strcmp(words[0], "cd") == 0) {
      int chdir_status;
      
      // cd funtionality
      if (words[2] != NULL) {
        perror("cd takes one argument but was given more than one");
        goto end_of_loop;
      }
      if (words[1] == NULL) {
        chdir_status = chdir(getenv("HOME"));

        if (chdir_status == -1) {
          perror("unable to change directory");
        }
        goto end_of_loop;
      } else if (words[1] != NULL) {

        chdir_status = chdir(words[1]);
        if (chdir_status == -1) {
          perror("unable to change directory");
        }
        goto end_of_loop;
      }
      goto end_of_loop;
    }

    else if (strcmp(words[0], "exit") == 0) {
      
      // exit functionality
      if (words[2] != NULL) {
        perror("too many arguments passed to exit, takes 1");
        goto end_of_loop;
      }

      if (words[1] == NULL) {
        goto exit;
      }
      else if (words[1] != NULL) {
        if (isdigit(words[1][0]) != 0 && atoi(words[1]) == 0) {  
          last_exit_int = 0;
        }
        else if (atoi(words[1]) == 0) {
          perror("argument passed to exit is not an integer");
          goto end_of_loop;
        } else {last_exit_int = atoi(words[1]);
        }
        goto exit;
      }
    }

    // parse tokenized commandline
    i= 0;
    char *input_file = "";
    char *output_file = "";
    int is_bg_bool = 0;
    int last_nonnull_idx = 0;

    // get to the end of the non-null words values
    while (words[i] != NULL) {
      if (strcmp(words[i], "#") == 0) {
        words[i] = NULL;
        continue;
      }
      i++;
    }

    last_nonnull_idx = i-1;

    if (last_nonnull_idx < 1) {
      goto nothing_to_parse;
    }
    
    i -= 1;

    // single pass begins      
    if(strcmp(words[i], "&") == 0) { 
      words[i] = NULL;
      is_bg_bool = 1;
    }

    if (last_nonnull_idx < 2) {
      goto nothing_to_parse;
    }

    if (strcmp(words[i-1], "<") == 0) {

      words[i-1] = NULL;
      last_nonnull_idx = i-2;
      input_file = strdup(words[i]);
    } else if (strcmp(words[i-1], ">") == 0) {
      words[i-1] = NULL;
      last_nonnull_idx = i-2;
      output_file = strdup(words[i]);
    }

    if (last_nonnull_idx < 4) {
      goto nothing_to_parse;
    }

    if (strcmp(words[i-3], "<") == 0) {
      words[i-3] = NULL;
      input_file = words[i-2];
      words[i-2] = NULL;
    } else if (strcmp(words[i-3], ">") == 0) {
      words[i-3] = NULL;
      output_file = words[i-2];
      words[i-2] = NULL;
    }

nothing_to_parse:

    // fork non-built-in commands
    int new_in_fd;
    int new_out_fd;
    int execute_return;
    pid_t spawn_pid = fork();

    switch (spawn_pid) {
    
      case -1:
        perror("fork() failed\n");
        exit(1);
        break;
      
      case 0:     // child executes
        if (strcmp(input_file, "") != 0) {   // if input file present, open new file descriptor
          close(0);
          new_in_fd = open(input_file, O_RDONLY);
        }

        if (strcmp(output_file, "") != 0) {     // if output file present, open new file descriptor, create new file if necessary
          close(1);
          new_out_fd = open(output_file, O_CREAT | O_WRONLY | O_TRUNC, 0777);
        }

        execute_return = execvp(words[0], words);   // send command and arguments to an exec function
        if (execute_return) {
          perror("execvp");
          goto end_of_loop;
        }

      default:    // parent executes
        if (is_bg_bool == 1) {  // check if background flag is set

          // update global variable in correct string format
          char bg_pid_str[8]; 
          sprintf(bg_pid_str, "%d", spawn_pid);
          bg_pid_ptr = bg_pid_str; 
          bg_pid = spawn_pid;

          sigaction(SIGINT, &SIGINT_action, &ignore_action); // set SIGINT to dummy handler behavior (makes it possible to send SIGINT signal in bg proc)
        }
        else {
          int child_fg_exit_status;

          waitpid(spawn_pid, &child_fg_exit_status, WUNTRACED); // blocking wait for fg process

          if (WIFEXITED(child_fg_exit_status)) {    // check if exited and update exit status global var accordingly
            last_exit_int = WEXITSTATUS(child_fg_exit_status);
          } 

          if (WIFSTOPPED(child_fg_exit_status)) {   // check if stopped and output appropriate message
            fprintf(stderr, "Child process %jd stopped. Continuing.\n", (intmax_t) spawn_pid);
            kill(spawn_pid, SIGCONT);
          }
          continue;
        }
    }

end_of_loop:
    fflush(stdout);
    fflush(stderr);
    continue;
  }

exit:
  kill(0, SIGINT);  // kills all child processes
  fprintf(stderr, "\nexit\n");
  exit(last_exit_int);

  return 0;
}



#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <wait.h>





// define the max size of the command line, usually when you run any kind of c
// program it runs its own terminal
//  returns with some value and exits
#define CMDLINE_MAX 512
#define MAX_ARGS 16
#define MAX_TOKEN_LENGTH 32



// we are changing the input in place. This is done via pass by reference. 

void cleanInput(char* input){
  //we will be making a copy of our command line input to be used 
  //for our linked list
  char temp[CMDLINE_MAX];
  strcpy(temp, input); // get whatever is in the input into temp
  //i = our index for the copy
  int i =0;
  //j = index for our command line input
  int j =0;//start in the beginning of the string 
  // while the copy does not encounter a null character 
  //and if the command line is less than 512:
  while(temp[i] != '\0'&& j < CMDLINE_MAX){
    //if we ever encounter any of these symbols, do the following:
    if(temp[i] == '<' || temp[i] == '>' || temp[i] == '|'){
      //once encountered, if the previous character is something other than a space. 
      if(temp[i-1] != ' '){
        input[j] = ' ';// at that position put a space.
        j++;// go to the next character in th
      }
      if(temp[i] == '>' && temp[i+1] == '>'){
        input[j] = '>';

        j++;
        i++;
      }
      input[j] = temp[i];

      j++;
      i++;
      if(temp[i] != ' '){
        input[j] = ' ';

        j++;
      }
    }
    else{
      input[j] = temp[i];
      j++;
      i++;
    }
  }
  input[j] = '\0'; // may have went past the old end of line, so must add in old end of line character. 
}

typedef struct Node {
  char **args;
  struct Node *next;
  int pid;
  int status;
} Node;

void printNode(Node* head);
// everytime we add a node to the list, we're going to return the list. Anytime
// the list is empty the new node will become the list.

// to do: explain how adding the node works 
Node *addNode(Node *head, Node *toAdd) {
  if (head == NULL) {
    head = toAdd;
  } else {
    head->next = addNode(head->next, toAdd);
  }
  //printNode(head);
  return head;
}

// if there is a node, deletes all the nodes after it, and then frees up the
// memory.

//to do:  explain how deletList works
void deleteList(Node *head) {
  if (head == NULL) {
    return;
  }
  deleteList(head->next);
  free(head->args);
  free(head);
}

void printList(Node* head){
  if(head == NULL){
    printf("list is empty");
  }
  while(head != NULL){
    printNode(head);
    //takes us to the next node in the linked list
    head = head->next;
  }
}

void printNode(Node *head) {
  printf("Command: %s\n", head->args[0]);
  printf("Arguments: \n");
  for (int i = 0; head->args[i] != NULL && i < MAX_ARGS; i++) {
    printf("- %s\n", head->args[i]);
  }
}
//explain how makeNode works but specifically how *arg works, 
//does the strtok(NULL, " ") keep track of the original string no matter where it's called
char *makeNode(Node *newNode, char *command) {
  newNode->next = NULL;
  // allocates memory for the arguments
  newNode->args = malloc(sizeof(char *) * MAX_ARGS);
  // initializes each argument to NULL to avoid randomized addresses
  for (int i = 0; i < MAX_ARGS; i++) {
    newNode->args[i] = NULL;
  }
  newNode->args[0] = command;
  int argCount = 1;
  // we use null here because strtok is already tokenizing the command line.
  char *arg = strtok(NULL, " ");
  while (argCount < MAX_ARGS && arg != NULL && strcmp(arg, "|") != 0 &&
         strcmp(arg, "<") != 0 && strcmp(arg, ">") != 0 && strcmp(arg, ">>") != 0) {
    newNode->args[argCount] = arg;
    argCount++;
    arg = strtok(NULL, " "); // move to next word in command line, strtok command gets the next ball. 
  }
  return arg; // we return arg here instead of node because the rest of the
              // program needs to know if arg is a redirection symbol or a
              // piping symbol.
}
// we can already use special commands and execute external programs.
// get familiar with: standard c library for pipe, execlp, fork, waitpid,



void pwd(){
  char cwd[512];

  getcwd(cwd, sizeof(cwd));

  printf("%s\n", cwd);
  printf("+ completed 'pwd' [0]\n");

}

int main(void) {
  // char** args;
  //  need to dynamically allocate 2d array to avoid warning about type in execv
  //  it exepcts a const char* [] which making args[X][Y] does not do
  char cmd[CMDLINE_MAX];
  char cmd_copy[CMDLINE_MAX];
  // args =  malloc(MAX_ARGS*sizeof(char*));
  // allocate space to hold pointers to the strings that are our arguments

  // we don't just want one command
  while (1) {

    // all these variables are initialized here because they are reset for each command. 
    char *nl;
    Node *head = NULL;
    char *outfile = NULL;
    int append = 0; //keep track of append vs write
    // int retval;

    /* Print prompt */
    printf("sshell$ ");
    fflush(stdout);
    // type in characters, waits for the buffer to be filled or \n. so We want
    // to print the prompt and we want the buffer for stdout to be empty
    // usually matters when taking input, so when taking input, we want the
    // buffer to be empty when you type on a keyboard, out input is stored in a
    // buffer until something needs it. fflush clears that buffer out in case
    // that anything is extraneous there.

    /* Get command line */

    fgets(cmd, CMDLINE_MAX, stdin);

    // gets input from the specified input buffer. In this case, stdin which is
    // the keyboard
    // whatever is copied from the buffer gets put into the cmd.

    /* Print command line if stdin is not provided by terminal */
    if (!isatty(STDIN_FILENO)) {
      printf("%s", cmd);
      fflush(stdout);
    }

    // stdin the keyboard, stdout is the screen, this is saying that if the os
    // did not provide stdin, in that case it prints the command to the screen.

    /* Remove trailing newline from command line */
    nl = strchr(cmd, '\n'); // finds the newline character of the command
    if (nl)
      *nl = '\0'; // finds the position of the newline character and sets it to
                  // null. Should be the last character of the command
    // make a copy for printing after the command is done that is not modified
    // by strtok

    //any spaces that are extra at the end will be gotten rid of
    int end = strlen(cmd) - 1;
    while(cmd[end] == ' '){
      cmd[end] = '\0';
      end--;
    }
    if(cmd[end] == '|'){
      fprintf(stderr, "Error: missing command\n");
      continue;
    }
    if(cmd[end] == '>'){
      fprintf(stderr, "Error: no output file\n");
      continue;
    }
    strcpy(cmd_copy, cmd);
   cleanInput(cmd);
    // split the command up into an array using whitespace to separate
    char *pch; 
    // count how many tokens were parsed
    // int argcount = 0;
    // gets the first token (splits on a space)

    pch = strtok(cmd, " ");
    // check for empty input
    if(pch == NULL) {
      fprintf(stderr, "Error: missing command\n");
      continue;
    }
    if(strcmp(pch, "|") == 0 || (strcmp(pch, ">") == 0 )|| (strcmp(pch, ">>") == 0) || (strcmp(pch, "<") == 0)){
      fprintf(stderr, "Error: missing command\n");
      continue;
    }
    //specifies if a file has been given
    int fileSpecified = 0;
    // used to restart while(1) loop when too many arguments are given
    int tooManyArguments = 0;
    // as long as there are tokens remaining
    while (pch != NULL) {
      if(fileSpecified == 1){
        fprintf(stderr, "Error: mislocated output redirection \n");
        deleteList(head);
        head = NULL;
        break;
      }
      Node *newNode = malloc(sizeof(Node));
      pch = makeNode(newNode, pch);// at this line, pch can only be one of 6 possible things, <,>,>>,|,NULL.
                                  //or if it is not one of those 5 above, then it is an extra argument. We know for certain that it is an extra argument because we have a while loop that specifically states that if the arguments are greater than 16 stop reading tokens. 

      head = addNode(head, newNode);

      
    
      if(pch != NULL && strcmp(pch, "<") != 0 && strcmp(pch, ">") != 0 && strcmp(pch, "|") && strcmp(pch, ">>") != 0){
        // print out error message in the example stating too many arguments
        //delete list head to avoid memory leaks and then continue
        fprintf(stderr, "Error: too many process arguments\n");
        deleteList(head);
        head = NULL; // we don't have any commands to run, break out of the list 
                    // then it sees we have no commands to run and then skip the rest, the execution phase 
        tooManyArguments = 1;
        break;
      }
      while (pch != NULL && (strcmp(pch, ">") == 0 || strcmp(pch, ">>") == 0)){
        if (strcmp(pch, ">") == 0) {
          outfile = strtok(NULL, " ");
          append = 0;
          fileSpecified = 1;
          pch = strtok(NULL, " ");
        }
        else if (strcmp(pch, ">>") == 0) {
          outfile = strtok(NULL, " ");
          fileSpecified = 1;
          append = 1;//this way we know that we are appending to a file
          pch = strtok(NULL, " ");
        }
      }
      if (pch != NULL && strcmp(pch, "|") == 0) {
        pch = strtok(NULL, " ");
      }
      
      // copy this token into the args array
      // args[argcount] = pch;
      // argcount++;
      // get the next token
      // pch = strtok(NULL, " ");
    }
    // if too many arguments were given, skip the rest of the processing
    /*Node* temp = head;
    while(temp != NULL){

      printNode(temp);
      temp = temp->next;
    }

    printf("end of list\n");
    */
    if(tooManyArguments == 1) {
      continue;
    }
    //must add a continue here in case the user never enters anything

    //first we check if we have a list of commands, if a file was 
    if(head == NULL){
      if(pch != NULL && strcmp(pch, ">") != 0 && strcmp(pch, "|") && strcmp(pch, ">>") != 0){
        if(fileSpecified == 0){
          fprintf(stderr, "Error: missing command\n");
      }

      }
      continue;
    }
    //handle built in commands
    if(strcmp(head->args[0], "pwd") == 0){
      pwd();
      deleteList(head);
      continue;
    }
    if(strcmp(head->args[0], "sls") == 0){
      DIR *pdir;
      struct dirent *pdirent;
      // open the current directory
      pdir = opendir(".");
      // loop through each entry in the directory
      // when we do pwd,
      while ((pdirent = readdir(pdir)) != NULL) {
        // TODO skip . and .. directories?
        // use stat to get the information about the file size
        struct stat results;
        if (stat(pdirent->d_name, &results) < 0) {
          printf("stat error\n");
        }
        printf("%s (%ld bytes)\n", pdirent->d_name, results.st_size);
      }
      closedir(pdir);
    continue;
    }
    if(strcmp(head->args[0], "exit") == 0){
      fprintf(stderr, "Bye...\n+ completed 'exit' [0]\n");
      // we are deleting the head everytime  after one of these commands in order to free up memory. 
      deleteList(head);

      break;
    }
    // attempt at redirection
    int currentOut;
    
    if(strcmp(head->args[0], "cd") == 0){
      int status = 0;

        // get the name of the directory
        char *pdir_name = head->args[1];
        printNode(head);
        //printf("handling built in command cd, pdir_name: %s\n", pdir_name);

        // TODO handle if cd does not have an argument or has too many arguments?
        if (chdir(pdir_name) < 0) {
          printf("Error: cannot cd into directory\n");
          status = 1;
        }
        printf("+ completed '%s' [%d]\n", cmd_copy, status);
      deleteList(head);
      continue;
    }


    //pid_t ppid;
    //int fds[MAX_ARGS][2];
    //int i = 0;
    Node* currentNode = head;
    if (currentNode->next != NULL){ // check for pipe
      while (currentNode != NULL) {
          currentNode = currentNode->next;
      }
    } 
    else {
        // the one with pid > 0 is the parent
      int pid = fork();
      if (pid == 0) {
        /* Child */
        if(outfile != NULL){
          if(append == 0){
            currentOut = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            dup2(currentOut, STDOUT_FILENO);
            close(currentOut);
            if(currentOut == -1){
              fprintf(stderr, "Error: cannot open output file\n");
              deleteList(head);
              head = NULL;
              break;
            }
          }
          else{
            currentOut = open(outfile, O_WRONLY | O_CREAT | O_APPEND, 0644);
            dup2(currentOut, STDOUT_FILENO);
            close(currentOut);
            if(currentOut == -1){
              fprintf(stderr, "Error: cannot open output file\n");
              deleteList(head);
              head = NULL;
              break;
            }
          }
        } 
        execvp(head->args[0], head->args);
        perror("execv");
        exit(1);

      } else if (pid > 0) {
        /* Parent */
        int status;
        waitpid(pid, &status, 0);
        fprintf(stderr, "* completed '%s' [%d]\n", cmd_copy, WEXITSTATUS(status));

      } else {
        perror("fork");
        exit(1);
      }
    }
    
    

    
    
    
  deleteList(head);
  }

   return EXIT_SUCCESS;
}
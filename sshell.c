#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <wait.h>
#include <sys/time.h>


#define CMDLINE_MAX 512
#define MAX_ARGS 16
#define MAX_TOKEN_LENGTH 32


void cleanInput(char* input){
  //we will be making a copy of our command line input to be used for our linked list
  char temp[CMDLINE_MAX];
  strcpy(temp, input); // get whatever is in the input into temp
  int i =0; // index for copy
  int j =0; // index for command line input
  while(temp[i] != '\0'&& j < CMDLINE_MAX){ // while the copy does not encounter a null character and if the command line is less than 512:
    if(temp[i] == '<' || temp[i] == '>' || temp[i] == '|'){
      //once encountered, if the previous character is something other than a space. 
      if(temp[i-1] != ' '){
        input[j] = ' '; // at that position put a space.
        j++; // go to the next character
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

typedef struct Node { // linked list for command parsing
  char **args;
  struct Node *next;
  int pid;
  int status;
} Node;

void printNode(Node* head);

// everytime we add a node to the list, we're going to return the list. Anytimee the list is empty the new node will become the list.
Node *addNode(Node *head, Node *toAdd) {
  if (head == NULL) {
    head = toAdd;
  } else {
    head->next = addNode(head->next, toAdd);
  }
  return head;
}

// if there is a node, deletes all the nodes after it, and then frees up the
// memory.

// delete the list when done and free memory
void deleteList(Node *head) {
  if (head == NULL) {
    return;
  }
  deleteList(head->next);
  free(head->args);
  free(head);
}

// used for error checking to see if the list was taken in correctly
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

// used for error echecking to see if node has correct arguments
void printNode(Node *head) {
  printf("Command: %s\n", head->args[0]);
  printf("Arguments: \n");
  for (int i = 0; head->args[i] != NULL && i < MAX_ARGS; i++) {
    printf("- %s\n", head->args[i]);
  }
}

// initialize the linked list
char *makeNode(Node *newNode, char *command) {
  newNode->next = NULL; // always intialize because memory is unpredicatble 
  newNode->args = malloc(sizeof(char *) * MAX_ARGS); // allocates memory for the arguments
  for (int i = 0; i < MAX_ARGS; i++) { // initializes each argument to NULL to avoid randomized addresses
    newNode->args[i] = NULL;
  }
  newNode->args[0] = command;
  int argCount = 1;
  char *arg = strtok(NULL, " "); // we use null here because strtok is already tokenizing the command line.
  while (argCount < MAX_ARGS && arg != NULL && strcmp(arg, "|") != 0 &&
         strcmp(arg, "<") != 0 && strcmp(arg, ">") != 0 && strcmp(arg, ">>") != 0) {
    newNode->args[argCount] = arg;
    argCount++;
    arg = strtok(NULL, " "); // move to next word in command line, strtok command gets the next ball. 
  }
  return arg; // we return arg here instead of node because the rest of the program needs to know if arg is a redirection symbol or a piping symbol
}

// prints the working directory
void pwd(){
  char cwd[CMDLINE_MAX]; 
  getcwd(cwd, sizeof(cwd)); // get the current working directory
  printf("%s\n", cwd);
  fprintf(stderr, "+ completed 'pwd' [0]\n");

}

int main(void) {
  char cmd[CMDLINE_MAX];
  char cmd_copy[CMDLINE_MAX];

  while (1) {

    // all these variables are initialized here because they are reset for each command. 
    char *nl;
    Node *head = NULL;
    char *outfile = NULL;
    int append = 0; //keep track of append vs write

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
    // gets input from the specified input buffer. In this case, stdin which is the keyboard
    // whatever is copied from the buffer gets put into the cmd.

    /* Print command line if stdin is not provided by terminal */
    if (!isatty(STDIN_FILENO)) {
      printf("%s", cmd);
      fflush(stdout);
    }

    /* Remove trailing newline from command line */
    nl = strchr(cmd, '\n'); // finds the newline character of the command
    if (nl)
      *nl = '\0'; // finds the position of the newline character and sets it to null. Should be the last character of the command
    
    // make a copy for printing after the command is done that is not modified by strtok
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
    if(cmd[end] == '>'){ //what is the last character of >>
      fprintf(stderr, "Error: no output file\n");
      continue;
    }
    strcpy(cmd_copy, cmd); // copy command into copy
    cleanInput(cmd); // clean the command line

    // split the command up into an array using whitespace to separate
    char *pch; 
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
      //or if it is not one of those 5 above, then it is an extra argument. 
      //We know for certain that it is an extra argument because we have a while loop that 
      //specifically states that if the arguments are greater than 16 stop reading tokens. 
      head = addNode(head, newNode);

      if(pch != NULL && strcmp(pch, "<") != 0 && strcmp(pch, ">") != 0 && strcmp(pch, "|") && strcmp(pch, ">>") != 0){
        // print out error message in the example stating too many arguments
        // delete list head to avoid memory leaks and then continue
        fprintf(stderr, "Error: too many process arguments\n");
        deleteList(head);
        head = NULL; // we don't have any commands to run, break out of the list 
                     // then it sees we have no commands to run and then skip the rest, the execution phase 
        tooManyArguments = 1;
        break;
      } 
      while (pch != NULL && (strcmp(pch, ">") == 0 || strcmp(pch, ">>") == 0)){
        if (strcmp(pch, ">") == 0) { 
          outfile = strtok(NULL, " "); // get outfile for output redirection
          append = 0; // we aren't appening in the case of >
          fileSpecified = 1; // we were specified a file
          pch = strtok(NULL, " "); // go to next string
        }
        else if (strcmp(pch, ">>") == 0) {
          outfile = strtok(NULL, " "); // get outfile for output redirection
          fileSpecified = 1; // we were specified a file
          append = 1;//this way we know that we are appending to a file
          pch = strtok(NULL, " ");
        }
      }
      if (pch != NULL && strcmp(pch, "|") == 0) {
        pch = strtok(NULL, " "); 
      }

    }
    // if too many arguments were given, skip the rest of the processing
    if(tooManyArguments == 1) {
      continue; //must add a continue here in case the user never enters anything
    }

    //if command is missing, print error and skip the rest of processing
    if(head == NULL){
        if(fileSpecified == 0){
          fprintf(stderr, "Error: missing command\n");
      }
      continue;
    }
    // check for built in commands
    if(strcmp(head->args[0], "pwd") == 0){
      pwd();
      deleteList(head);

      head = NULL;
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
        // use stat to get the information about the file size
        struct stat results;
        if (stat(pdirent->d_name, &results) < 0) {
          printf("stat error\n");
        }
        if(strcmp(pdirent->d_name, ".") != 0 && strcmp(pdirent->d_name, "..")){
          printf("%s (%ld bytes)\n", pdirent->d_name, results.st_size);
        }
        
      }
      closedir(pdir); // close when done
      continue;
    }
    if(strcmp(head->args[0], "exit") == 0){
      fprintf(stderr, "Bye...\n+ completed 'exit' [0]\n");
      // we are deleting the head everytime after one of these commands in order to free up memory. 
      deleteList(head);
      head = NULL;
      break;
    }
    if(strcmp(head->args[0], "cd") == 0){
      int status = 0;
        // get the name of the directory
      char *pdir_name = head->args[1];
      if (chdir(pdir_name) < 0) {
        fprintf(stderr, "Error: cannot cd into directory\n");
        status = 1;
      }
      fprintf(stderr, "+ completed '%s' [%d]\n", cmd_copy, status);
      deleteList(head);
      head = NULL;
      continue;
    }

    int currentIn = 0;
    int currentOut = STDOUT_FILENO;
    Node* current = head;
    while(current != NULL){
      // cleaning up pipes from previous command. This is because if there are mutiple commands we want to clean up previous commands redirect.
      // for the last process in the chain

      //declare pipes in order to use in the program. 
      // assigned in different condition, it will die if it inside the if statement, we want it to survive outside of it. 
      int pipes[2];
      if(current->next == NULL){
        //last command outputs to either stdout or file.
        //if we have a file we want to write to.
        if(outfile != NULL){
          if(append == 0){
              currentOut = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644); 
            if(currentOut == -1){
              fprintf(stderr, "Error: cannot open output file\n");
              deleteList(head);
              head = NULL;
              break;
            }
          }else{
            currentOut = open(outfile, O_WRONLY | O_CREAT | O_APPEND, 0644);//instead of delelting contents we append
            if(currentOut == -1){
              fprintf(stderr, "Error: cannot open output file\n");
              deleteList(head);
              head = NULL;
              break;
            }
          }
        }else{
          currentOut = STDOUT_FILENO; 
        }
      }else{
        pipe(pipes); // provides the pipes to connect programs together. 
        currentOut = pipes[1]; // 1 is write end of the pipe, so when we get here we are saying that the fd for currentOut is the write part of pipe.
      }//^choosing which output will be used and putting it into currentOut. i.e screen, file or pipe
      //connect new pipes and clean up old pipes. 
      //call fork here because we need to set up the pipes to connect the process. The pipes that connect the processes have to be created befoe fork
      // because otherwise the processe screated by fork won't know about the pipes so won't be able to talk to each other
      current->pid = fork();
      if (current->pid == 0) {
        /* Child */
        if(current->next != NULL){ // check if we pipe or not
          close(pipes[0]);
        }
        dup2(currentIn, 0); // standard inputput will point to what currentIn will be assigned to. 
        if(currentIn != 0){ // at this point we have 2 file descriptors for a file, we don't need it anymore. 
          close(currentIn);
        }
        //setting up next pipe or output
        dup2(currentOut, STDOUT_FILENO); 
        if(currentOut != STDOUT_FILENO){
          close(currentOut);
        }
        //execvp will run a process if it doesn't get a valid process then it will print an error statement.
        execvp(current->args[0], current->args);
        // at this point, execvp takes control, replaces the current process image. It will only return on an error. 
        fprintf(stderr, "Error: command not found\n");
        //exit child process
        exit(1);
      } else if (current->pid < 0) {
        perror("fork");
        exit(1);
      }else{
        if (currentOut != STDOUT_FILENO) { 
            close(currentOut); //don't want parent to hold onto it. close the write end of the pipe for the parent
        }
        //pid is the most recently executed program
        //keeping track of the most recent pid. 
        //keep the last pid to waitpid on the last program later. 

        if(currentIn != 0){
          close(currentIn);
        } // parent has a copy of the file descriptor that it doesn't need, if currentIn = stdin, don't close it else it should close

        //when creating the pipes between the two commands, we don't want to change the current in until we reach the next command. 
        //this command's output will be the next command's input.
        //setting up the next program so that it read from the read part of pipes. 
        current = current->next; // move onto next process, traverse linked list, iterate. 
        currentIn = pipes[0]; // force the next program's input to be read from the pipe of the program that just ran. This pipe belongs to the program above. 
      }
    } // executed the full command line, including piping and redirection
          
    //once here all programs have been started
    // head is set to null above if there was an error when trying to open the output file, which would have resulted in aborting trying to run the commands
    // so don't need to do anythig else, just start the while(1) loop back over
    if(head == NULL){
      //none of this code below has any meaning if head == NULL
      continue;
    }
    
    //so we wait for the last program to finish.
    // have to wait for the last process first, so we know everything is done executing. 
    // restarting back to the head of the linked list, to collect status information for all the processes ran. 
    current = head;
    //update the status of each program, after the last program has finished. 

    while(current != NULL){
      //waiting or not waiting for a program to end. 
      //always wait for the last program in the chain to finish. 
      //waitpid gives us the statuses, the return values of the program. 
      //waiting for all other processes to finish. 
      waitpid(current->pid, &(current->status), 0);

      current = current->next;
    }
    fprintf(stderr, " completed '%s'", cmd_copy);
    current = head;
    // since the status of the last process is saved in 'status' not in the linked list
    // print everything except the last process status out of th elinked list, then
    // print status on its own
    while(current != NULL){
      fprintf(stderr, "[%d]", WEXITSTATUS(current->status));
      current = current->next;
    }
    fprintf(stderr, "\n");
        //reconnect stdin and stdout to the originals. And then closing the copies. 
        deleteList(head);
        head = NULL;
      }

      return EXIT_SUCCESS;
    }


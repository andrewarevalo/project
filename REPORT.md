#Project 1

## Summary
This program, `sshell`, acts like a linux terminal and can take in several different basic commands. 


## Implementation of Project 1

Project 1 had a few big parts that we needed to implement. 

The first one is parsing the command line. We did this by using a linked list and putting each different command into a node of the linked list. We first cleaned up the command line to account for situations where there was no space between the pipe and a command by adding spaces between all the symbols. Then, we split up the commands on whitespace, which was stored in a string array for each actual command.  For example, if the command line we received was *cat file | grep hello* we would have a linked list: *node1 -> node2* with node one having the arguments *cat file* and node2 having the arguments *grep hello*. This implementation was used so that we could keep track of multiple commands for piping. 

Another big part of the implementation was the built-in commands: *cd, sls, pwd*. These were pretty simple to implement: we had these run before the fork since they do not require execvp() to run. Since we did not have to implement them with piping, we could do a simple check to see if the command was called correctly and run the function. *cd* had to be run in the parent process, because if it was run in the child process then the parent process wouldn't remember what directory to be in after the child process finished execution.

The next big part of the implementation was output redirection. For output redirection, we took in the output file specified and switched the stdout file descriptor to point to the given output file and run the command normally. This would change the output from the terminal to the given output file. 

The final big part of the implementation was piping. This was the most tricky part of the project, especially for more than two pipes. To implement a command like *cat /dev/urandom | base64 -w 80 | head -5*, we would need 2 different pipes. One where the first command points the the write of the first pipe and the second command points to the read of the first pipe. The second command would then point to the write of the second pipe and the third command would point to the read of the second pipe. 

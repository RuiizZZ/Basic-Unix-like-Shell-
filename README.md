# Basic-Unix-like-Shell


## Summary

This program, `sshell`, is a simple shell that accepts user input from the 
command line and executes it. It takes in one or more optional arguments. 
It supports typical built-in commands, redirection of standard output of command
to files, composition of commands via piping, as well as redirection of the 
standard error of commands to files or pipes, and simple ls-like buildin command. 

## Implementation

The implementation of this program has two main parts:

1. Parsing given command line into executable command and arguements 
2. Identifying special builtin commands such `exit`, `pwd`, and `cd`, speical 
symbols such as `>`, `<`, and `|` and perform actions accordingly

### Parsing options

At the beginning of program execution, `sshell` runs function `arg_parser()` 
which parses the incoming command line (i.e., `cmd`) into command and arguments 
(i.e., `command` and `arg[i]`).
All command line information is stored in an object data structure called 
`struct cmdLine`. We can then access this information by passing the object 
into our functions.
On occasions when special commands are detected in arguement, function 
`command_handler()` is executed. `command_handler()` contains all function calls
to perform builtin commands: exit, cd, pwd, and directory stack interactions.
Otherwise, function `system_call()` is executed. `system_call()` contains all 
function calls to perform specific command executions using external executable 
files.
`execvp()` sereaches for the file path automatically for execution and take in 
char *ptr arguements.

### Redirection
Upon program excution `arg_parser()` identifies the `>` or `<` symbol. It stores
 the immediate arguemnt after the symbol into `file`. This will later be used as
  filename for file manipilation.

### Piping
The parsing for piping takes two steps. First, `arg_parser()` is called to parse
 the command line into processes. `arg_parser()` is called again in `piping()` 
 where each process is parsed into command and arguments.
Details...
use of for loop
1. Create pipes and file descriptors; the number of them are equal to the number
 of pipe signs in command.
2. Use of for loop to iterate each process.
3. For each iteration, fork once. 
4. In the child section, send fd[0] to STDIN, and if it is the last process that
need to be executed, send fd[1] to STDOUT. In this way, the previous process's 
output could be the current process's input. 
5. In the parent section, close the previous file descriptor for security.

### Directory Stack
Directory is stored in a stack, implemented using a singly linked list `struct 
ListNode`.  A singly linked list allows an arbitrary number of paths and fast 
access.
This singly linked list contains two pointers, one to store the current 
directory, the other points to the next object.
Directory stack `struct listNode *ds` serves as the head node. 
New node is created in `push_directory()`. It is appended after `ds`, with its 
next pointer pointing to the previous directory. This way the stack keeps track 
of a history of directory, from the most recent down to the first.

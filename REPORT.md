# Project 1 SShell Report

#### General Information
  - Authors: Kyle Wang and Patrick Ong
  - Source Files: sshell.c, command.h, command.c

# High Level Design Choices
Our sshell program is split into two separate files, sshell.c and command.c. In sshell.c we have the main control flow of our program. We will handle user input and decision making within our main function. Depending on the the user input we have written functions that will handle all the different commands that users can give to our shell. 

In command.c and command.h, we created two structs, Command and Pipes. After reading a user command, our first decision is to check for input redirection, output redirection, and pipes. If any of these options appear in the command string, we will turn the boolean for specific command to true. We than parse the command string in to a Command struct and then perform necessary control flow to handle the command. The Pipes struct will be used when we need to handle piping. If pipes exist in a user command, we will parse it into our Pipes struct, into an array of Command structs. 

```
typedef struct command{
	char *command;
	char *arguments[MAX_COMMAND_ARGUMENTS];
	char *allCommands[MAX_COMMAND_ARGUMENTS];
	int numberCommands;
	bool inputRedirection; //true if their is <
	bool outputRedirection; //true if their is >
	bool pipe; //true if their is a pipe
	bool background; //true if their is background command
}Command;

typedef struct pipesequence{
	Command *commandArray[MAX_COMMAND_ARGUMENTS]; 
	char *originalArray[MAX_COMMAND_ARGUMENTS];
	char *originalUserInput;
	int pipeCount;
	int argumentCount;
}Pipes;
```
# Implementation Details

##### Parsing and Initializing Command
After creating our Command Struct, we pass our struct into our parser along with the original command with blank spaces replaced with semicolon. This decision was made because we were told that parsing by blank spaces is very dangerous. The parser uses strtok to parse by semicolons. The parser parses the entire line and appropriately adds the necessary data to the command struct. The Command struct had to be repeatedly changed because we needed to add new member variables to it in order to fulfill the needs of distinguishing more commands.
```
void parseCommand(Command *commandStruct, char* originalCommand)
```

##### Phase 3: Arguments
After parsing our command into our Command Struct, we have a function processCommand(), that will execute a command, using the fork+exec_wait method, if none of the boolean variables in our Command struct were true. This performs a simple execution using execvp if there are arguments for our command, and will execute execlp if their exist no arguments in our command.
```
void processCommand(const Command *commandStruct, char *originalCommand,char *filename);
```

##### Phase 4: Builtin Commands
In this phase, we did not have to use fork+exec+wait method. Instead we added checks in our main to check if user input matched those of the builtin commands. When user types the exit command, we print "Bye..." to stderr and break out of our main while loop and return 0 to exit our sshell. For the command pwd, we have the function processPWD(), that simply calls the getcwd() function and print the name of the working directory to standard out. For the command cd, we have the function processCD(). In here we call the function chdir(), and pass end the desired directory the user wishes to change to, which has already been parsed into our Command struct so we can access it from there. A status is collected from chdir, and if there is an error we print out an error to stderr.  
```
int processPWD();
void processCD(Command *command,char *originalCommand);
```
##### Phase 5: Input Redirection
If the user command included an input redirection symbol, the Command struct would have inputRedirection boolean as true. We needed to add another test case when reading the user input to check for '<'. We handled this in a similar way to handling blank lines, except we added semicolon surrounding the '<'. This was done so that our parser would place '<' in its own array index in the Command struct. Our control flow will than process it in our processInputRedirection() function. Inside our function we first replace the '<' symbol with a NULL, so when we pass it into exevp we will not be passing in '<' as an argument. Than we check to make sure that we have a valid command. This is done by making sure that there exist a argument after the '< ' symbol:
```
void processInputRedirection(Command *command, char *originalCommand,char *filename){ 
...
 for(i = 0; i < command->numberCommands; i++){
    	if (strcmp(command->allCommands[i], ">") == 0 && i != 0){
        	command->allCommands[i] = NULL; 
		if (command->allCommands[i+1] == NULL){
			fprintf(stderr,"Error: no output file\n");
			return;
		}
    	break;
	}	
...
}
```
If the command was a valid input redirection command, we'll open the file and place give it a file descriptor,fd. If there is and error with opening the file, print to stderr error prompt. We then close STDIN_FILENO and set fd as STDIN. Than we perform the command execution using execvp. 

##### Phase 6: Output Redirection
Similar to the implementation for phase 5, we make sure that '>' gets its own index in our allCommands[] in the Command struct and inside our outputRedirection() replace '>' in our allCommands[] with NULL and check if their is a valid argument after the '>' symbol. We than open the file, fd, and if there is no error, we dup fd onto STDOUT_FILENO and than execute using execvp

```
void processOutputRedirection(Command *command, char *originalCommand,char *filename)
```

##### Phase 5 & 6: Handling Input and Output Redirection In the Same Command
In order to deal with both input and output redirection, we implemented a function that combined the functionality of what we did in phase 5 and 6. There are two for loops, one with variable i and variable j. This is used to keep track of where in the array of commands the '<' and '>' symbol are respectively. We perform the same checks to see if there is a valid out and input file. Which ever symbol shows up first, we set that symbol to NULL in the allCommands array, so we could terminate at that point in the allCommands array:
```
command->allCommands[i] = NULL;
command->allCommands[j] = NULL;
```
Using the indexes i and j, we open the appropriate files and dup our input files onto STDIN_FILENO and output file onto STDOUT_FILENO.

```
fdout = open(command->allCommands[i+1], O_WRONLY | O_CREAT, 0777 );
fdin = open(command->allCommands[j+1], O_RDONLY);
dup2(fdin, STDIN_FILENO);
dup2(fdout, STDOUT_FILENO);
```
Than we execute using the new file descriptors. 

##### Parsing and Initializing Pipes
The creation of a Pipes struct happens if the command we read in has the symbol '|'. We create Pipes struct, initialize it, and pass it into our processPipe() function. In our parser, we used strtok to separate each part of the pipe with the '|' symbol. We store the results in Pipes member, originalArray[]. Than loop through originalArray and initialize the Command *commandArray[] in Pipes by passing the members into the processCommand() function. This well produce the array of Command structs that we need. 

```
void parsePipe(Pipes* pipe, char *originalCommand,char* originalUserInput)
```

##### Phase 7: Pipeline Commands
After parsing our command line with pipes into a Pipes struct, we pass the struct into our processPipe() function. The most important factor in implementing the piping is using the 'save' variable to be able to save our output. We have a struct Command** that points to the array of Command struct in our pipes struct. Our for loop keeps incrementing down this array of commands until there are no more commands to execute. Most important is having: 
```
dup2(save, 0)
save = fd[0]
```
If there was any output from a previous command, dup2(save,0) will make sure that we'll be reading from there. Having save = fd[0] will save the output that we might need. The if statement inside the child will set our write pipe to stdout when we have reached the last command and need to print to stdout. All the commands that are executed inside processPipeCommand(). This function provides checks to see if there input redirection in the first command of the pipe, or if their is out redirection in the last command of the pipe. If their is, we call the necessary function, processOutputRedirection() or processOutputRedirection(). 
```
void processPipeCommand(Command **command, char **originalCommand, const int count)
void processPipe(Pipes* pipes)
```

##### Phase 8: Background Commands


# Testing
Tests were done using the csif machines.Manual testing was done along the way by making comparison with the sshell_ref. Tests were also ran with test_sshell_student.sh. 

#Sources Used
- www.piazaa.com
- https://www.tutorialspoint.com/c_standard_library/c_function_strtok.htm
- https://www.tutorialspoint.com/c_standard_library/c_function_strcmp.htm
- https://www.tutorialspoint.com/c_standard_library/c_function_isspace.htm
- https://www.gnu.org/software/libc/manual/html_mono/libc.html#
- The C Programming Language by Brian Kernighan and Dennis Ritchie



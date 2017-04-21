#ifndef COMMAND_H_   /* Include guard */
#define COMMAND_H_

#define MAX_COMMAND_ARGUMENTS 16

typedef struct command{
	char *command;
	char *arguments[MAX_COMMAND_ARGUMENTS]; //include everything except for command
	char *allCommands[MAX_COMMAND_ARGUMENTS]; //include everythingin the command line
	int numberCommands;
	bool inputRedirection; //true if their is <
	bool outputRedirection; //true if their is >
	bool pipe; //true if their is a pipe
	bool background; //true if their is background command
	int count;
}Command;


typedef struct pipesequence{
	Command *commandArray[MAX_COMMAND_ARGUMENTS]; //array of Commands
	char *originalArray[MAX_COMMAND_ARGUMENTS]; //need this to intialize commandArray
	char *originalUserInput; //original user command
	int pipeCount;
	int argumentCount;
}Pipes;

void parsePipe(Pipes *pipe, char* originalCommand,char* originalUserInput);
void pipeInitialize(Pipes *pipe);
void commandInitialize(Command *commandStruct);
void parseCommand(Command *commandStruct, char* originalCommand);
#endif // COMMAND_H_

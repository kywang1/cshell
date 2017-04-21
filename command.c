#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "command.h"

void parseCommand(Command *commandStruct, char* originalCommand){
  char *command;
  command = malloc(sizeof(char)*strlen(originalCommand));
  strcpy(command,originalCommand);

  const char s[] = ";\n"; //parse by semi colons
  char* token = (char *) malloc(MAX_COMMAND_ARGUMENTS);
  token  = strtok(command,s);
  int i = 0;
  int k = 0;
  int j = 0;

  while(token != NULL){
    commandStruct->allCommands[j] = token;
    j++;

    if(i == 0){
      commandStruct->command = token;
      i++;
    }
    else{
      commandStruct->arguments[k] = token;
      k++;
    }
    commandStruct->numberCommands++;
    token = strtok(NULL,s);

  }
  commandStruct->arguments[commandStruct->numberCommands-1] = NULL;
  commandStruct->allCommands[commandStruct->numberCommands] = NULL;

  //add this b/c no way for individual commands in pipe to know this, so 
  //perform check again for each command in the command array for pipes
  char* commandPointer;
  commandPointer = originalCommand;

  while(*commandPointer != '\0'){  //add this check for parsing a command part of a pipe.
      if(*commandPointer == '<'){  //since each seperate command in a pipe cant check for this in main
                commandStruct->inputRedirection = true;
      }

      if(*commandPointer == '>'){
                commandStruct->outputRedirection = true;
      }
      commandPointer++;
  }

  if(strcmp(commandStruct->allCommands[commandStruct->numberCommands-1],"&") == 0){ //check if last command tells 
  	commandStruct->background = true;                                               // to run background command
  	commandStruct->allCommands[commandStruct->numberCommands-1] = NULL;
  }else{
	 commandStruct->background = false;
  }

  free(token);
}

void parsePipe(Pipes* pipe, char *originalCommand,char* originalUserInput){
	char *command;
	command = malloc(sizeof(char)*strlen(originalCommand));
	strcpy(command,originalCommand);

  	const char s[] = "|\n";
	char *token = (char *) malloc(MAX_COMMAND_ARGUMENTS);
	token = strtok(command,s); //return first part of the pipe

	int i = 0;
	while(token!= NULL){
		pipe->originalArray[i] = token; //parse command into original array
		token = strtok(NULL,s); //get the next part of the pipe
		pipe->pipeCount++;
		pipe->argumentCount++;    //echo;hi;;|;\n
		i++;
	}
  
  	pipe->pipeCount--; //subtract one b/c parser counted \n 

	for(i = 0; i< pipe->argumentCount;i++){
		parseCommand(pipe->commandArray[i],pipe->originalArray[i]); //parse command in originalArray into command array 
                                                                //parseCommand function
		pipe->commandArray[i]->count = i;
	}

  	pipe->commandArray[pipe->pipeCount+1] = NULL;
    	pipe->originalUserInput = originalUserInput;
    	strtok(pipe->originalUserInput,"\n");
  	free(token);
	
	
}

void pipeInitialize(Pipes *pipe){
	int i;
	for(i = 0 ; i < MAX_COMMAND_ARGUMENTS ; i++){
		pipe->commandArray[i] = (Command*) malloc(sizeof(Command));
		pipe->originalArray[i] = (char*) malloc(sizeof(char));
	}
  pipe->argumentCount = 0;
	pipe->pipeCount = 0;
	
}

void commandInitialize(Command *commandStruct){
	commandStruct->numberCommands = 0;
	commandStruct->command = NULL;
	commandStruct->inputRedirection = false;
	commandStruct->outputRedirection = false;
	commandStruct->pipe = false;
	commandStruct->background = false;
	int i;
	for(i = 0;i<MAX_COMMAND_ARGUMENTS ; i++){
	commandStruct->arguments[i] = NULL;
	commandStruct->allCommands[i] = NULL;
	}
}

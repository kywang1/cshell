#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "command.h"

#define MAX_ARGUMENTS 256
#define MAX_BLOCK_SIZE unsigned long long int

void processCommand(const Command *commandStruct, char *originalCommand,char *filename);
int processPWD();
void processCD(Command *command,char *originalCommand);
void processBiRedirection(Command *command, char *originalCommand,char *filename);
void processInputRedirection(Command *command, char *originalCommand,char *filename,bool pipe);
void processOutputRedirection(Command *command, char *originalCommand,char *filename, bool pipe);
void processPipe(Pipes *pipes);
int processPipeCommand(Command **command, char **originalCommand, const int count);

int main(int argc, char *argv[])
{

	while(1){
		char* myCommand;
		char buffer[256];
		myCommand = (char*)malloc(1000);

		printf("sshell$ "); //prompts user for command

		//getdelim(&myCommand,&nbytes,'\n',stdin); //get command into myCommand
					 //extra space are \n
		//strtok(myCommand,"\n"); //remove the \n at the end of every string, causing conflict in execlp
		fgets(myCommand,sizeof(buffer),stdin);
		if(!isatty(STDIN_FILENO)){
			printf("%s",myCommand);
			fflush(stdout);
		}

        	Command commandStruct;
		commandInitialize(&commandStruct);

		// replace all spaces with ; because using strtok on our parser with space is dangerous
		char* commandPointer = (char *)malloc(sizeof(char));
		int i = 0;
		commandPointer = myCommand;
		char *newCommand = (char*)malloc(512);
		while(*commandPointer != '\0'){
			
			if(isspace(*commandPointer)){
				newCommand[i] = ';';
			}
			else{
				newCommand[i] = *commandPointer;
			}

			if(*commandPointer == '<'){ //want to get this as its own char* in our Command Struct
				commandStruct.inputRedirection = true; //so add semi colons before and after it
				newCommand[i] = ';';
				newCommand[i+1] = *commandPointer;
			       	newCommand[i+2] = ';';
				i+=2;
			}

			if(*commandPointer == '>'){
				commandStruct.outputRedirection = true;
			       	newCommand[i] = ';';
				newCommand[i+1] = *commandPointer;
				newCommand[i+2] = ';';
				i+=2;
			}

			if(*commandPointer == '|'){
				commandStruct.pipe = true;
				newCommand[i] = ';';
				newCommand[i+1] = *commandPointer;
				newCommand[i+2] = ';';
				i+=2;
			}
			
			if(*commandPointer == '&'){
				newCommand[i] = ';';
				newCommand[i+1] = *commandPointer;
				newCommand[i+2] = ';';
				i+=2;
			}

			commandPointer++;
			i++;
		}
		//Parse command into command Struct
		parseCommand(&commandStruct,newCommand);

		//generate file path
		char filepath[MAX_ARGUMENTS];
		strcpy(filepath,"/usr/bin/");
		strcat(filepath,commandStruct.command);


		if(strcmp(commandStruct.command,"exit")==0){
			fprintf(stderr,"Bye...\n"); // print to stderr
			break;
		}
		else if(strcmp(commandStruct.command,"cd")==0){
        		processCD(&commandStruct,myCommand);
		}
		else if(strcmp(commandStruct.command,"pwd")==0){
		    	processPWD();
		}
		else if(commandStruct.inputRedirection && commandStruct.outputRedirection && commandStruct.pipe!=true){
		    	processBiRedirection(&commandStruct,myCommand,filepath);
		}
		else if(commandStruct.inputRedirection && commandStruct.pipe!=true){
		    	processInputRedirection(&commandStruct,myCommand,filepath,false);
		}
		else if(commandStruct.outputRedirection && commandStruct.pipe!=true){
		    	processOutputRedirection(&commandStruct,myCommand,filepath,false);
		}
		else if(commandStruct.pipe){
			Pipes pipe;
			pipeInitialize(&pipe);
			parsePipe(&pipe,newCommand,myCommand);
			processPipe(&pipe);
		}
		else{
			processCommand(&commandStruct,myCommand,filepath); //process command
		}
		
		free(myCommand); //free memory allocation for myCommand
		
		for(i = 0; i < 512 ; i++){ //re-initialize newCommand for new user input
			newCommand[i] = (MAX_BLOCK_SIZE) NULL;
		}
	}
	return 0;
}


void processCommand(const Command *command, char *originalCommand,char *filename){
	int status = 0;
	strtok(originalCommand,"\n");

    	if(fork()!= 0){
        	//parent
		if(!command->background){
			waitpid(-1,&status,0);
		}
		else{
			waitpid(-1, &status, WNOHANG);
		}
    	}else{
    		if(command->numberCommands >=2){
 			status = execvp(filename,command->allCommands);
    		}
    		else{
    			status = execlp(filename,command->command,NULL);
    		}
        	exit(1);
    	}

    	if(status != 0){ //check for error command
    		fprintf(stderr,"Error:command not command\n");
    	}else if(!command->background){
    		fprintf(stderr,"+ completed '%s' [%d]\n",originalCommand,status );
    	}	

}

int processPWD(){
	char* currentDirectory;
	char buffer[MAX_ARGUMENTS];

	currentDirectory = getcwd(buffer,MAX_ARGUMENTS); //get currenDirectory
	fprintf(stdout,"%s\n", currentDirectory);
	fprintf(stderr,"+ completed 'pwd' [0]\n");
	return 0;
}

void processCD(Command *command, char *originalCommand){ //directory = commandStruct.arugments[0]
    	//ENOTDIR
    	strtok(originalCommand,"\n");
   	int status;
	status = chdir(command->arguments[0]); //call chdir on command struct arguments, cd is only given one parameter
	if(status != 0){ //check return status for error
		fprintf(stderr,"Error: no such directory\n");
		fprintf(stderr,"+ completed '%s' [%d]\n",originalCommand,1 );
	}
	else{
		fprintf(stderr,"+ completed '%s' [%d]\n",originalCommand,status );
	}
}

void processBiRedirection(Command *command, char *originalCommand,char *filename){
 
	int status = 0;
	strtok(originalCommand,"\n");

	int i; //output
	for(i = 0; i < command->numberCommands; i++){ //look for index of >
		if (strcmp(command->allCommands[i], ">") == 0 && i != 0){
			if (command->allCommands[i+1] == NULL){
				fprintf(stderr,"Error: no output file\n");
				return;
			}
			break;
		}				
   	 }

	int j; //input
	for(j = 0; j < command->numberCommands; j++){//look for index of <
		if (strcmp(command->allCommands[j], "<") == 0 && j != 0){
			if (command->allCommands[j+1] == NULL){
				fprintf(stderr,"Error: no input file\n");
				return;
			}
			break;
		}				
    	}

	command->allCommands[i] = NULL; //set to NULL the index to run execvp
	command->allCommands[j] = NULL; //




	int fdout;
	//creates file if does not exist and gives permission
	fdout = open(command->allCommands[i+1], O_WRONLY | O_CREAT, 0777 ); //O_CREATE

	if (fdout == -1){
		fprintf(stderr,"Error: cannot open output file\n");
		return;
	}

    	if(fork()!= 0){
        //parent
    		waitpid(-1,&status,0);
    	}
	else{
		int fdin;
		fdin = open(command->allCommands[j+1], O_RDONLY); 
		if (fdin == -1){
	    		fprintf(stderr,"Error: cannot open input file\n");
	    		return;
        	}
		dup2(fdin, STDIN_FILENO); //set input file as stdin
		//
		dup2(fdout, STDOUT_FILENO); //set output file as stdout 	

    		if(command->numberCommands >=2){
 			status = execvp(filename,command->allCommands);
    		}
    		else{
    			status = execlp(filename,command->command,NULL);
    		}	
        	exit(1);
    	}

    	close(fdout);

    	if(status != 0){ //check for error command
    		fprintf(stderr,"Error:command not command\n");
    	}
	else{
    		fprintf(stderr,"+ completed '%s' [%d]\n",originalCommand,status );
    	}
}

void processInputRedirection(Command *command, char *originalCommand,char *filename, bool pipe){

	int status = 0;
	strtok(originalCommand,"\n");

	if(fork()!= 0){
		//parent
		waitpid(-1,&status,0);
	}
	else{
		int i;
		for(i = 0; i < command->numberCommands; i++){
			if (strcmp(command->allCommands[i], "<") == 0 && i != 0){
				command->allCommands[i] = NULL; //set to NULL so we when we pass into execvp it terminates here.
				if (command->allCommands[i+1] == NULL){ //no input file
					fprintf(stderr,"Error: no input file\n");
					return;
				}
				break;
			}				
		}

		int fd;
		fd = open(command->allCommands[i+1], O_RDONLY);
		if (fd == -1){
		    printf("Error: cannot open input file\n");
		    return;
		}

		dup2(fd, STDIN_FILENO); //close stdin and replace with fd
		execvp(filename,command->allCommands);
	    	if(command->numberCommands >=2){
	 		status = execvp(filename,command->allCommands);
	    	}
	    	else{
	    		status = execlp(filename,command->command,NULL);
	    	}

		exit(1);
    	}

	if(!pipe){
		if(status != 0){ //check for error command
			fprintf(stderr,"Error:command not command\n");
		}
		else{
			fprintf(stderr,"+ completed '%s' [%d]\n",originalCommand,status );
		}
	}

}

void processOutputRedirection(Command *command, char *originalCommand,char *filename,bool pipe){

	int status = 0;
	strtok(originalCommand,"\n");

	int i;
	for(i = 0; i < command->numberCommands; i++){
		if (strcmp(command->allCommands[i], ">") == 0 && i != 0){
        		command->allCommands[i] = NULL; // set to NULL so we when we pass into execvp it terminates here.
			if (command->allCommands[i+1] == NULL){ //no output file
				fprintf(stderr,"Error: no output file\n");
				return;
			}
			break;
		}				
    	}

    
    	int fd;
	//creates file if does not exist
    	fd = open(command->allCommands[i+1], O_WRONLY|O_CREAT, 0777);
    
    	if (fd == -1){
		fprintf(stderr,"Error: cannot open output file\n");
		return;
    	}

    	if(fork()!= 0){
        	//parent
    		waitpid(-1,&status,0);
    	}
	else{
		//
		dup2(fd, STDOUT_FILENO);  	

    		if(command->numberCommands >=2){
 			status = execvp(filename,command->allCommands);
    		}
    		else{
    			status = execlp(filename,command->command,NULL);
    		}
        	exit(1);
    	}	

    	close(fd);

	if(!pipe){
	    	if(status != 0){ //check for error command
	    		fprintf(stderr,"Error:command not command\n");
	    	}
		else{
	    		fprintf(stderr,"+ completed '%s' [%d]\n",originalCommand,status );
	    	}
	}
}

void processPipe(Pipes* pipes){
	if(pipes->pipeCount +1  != pipes->argumentCount){
		fprintf(stderr,"Error: not command line\n");
	}
	else{		

		int fd[2];
		int pid;
		int save = 0;
		Command** command = (Command**) malloc(sizeof(Command));
		command = pipes->commandArray;

		char** originalCommand = (char**) malloc(sizeof(char));
		originalCommand = pipes->originalArray;

		int i;
		int statusArray[16];
		
		for(i = 0; *command != NULL; command++,i++){

				pipe(fd);
		      	pid = fork();
		      	if (pid != 0) { //parent
			  	waitpid(-1,&(statusArray[(*command)->count]),0);
	 			close(fd[1]); //close write pipe
		  		save = fd[0]; //save our output to be processed as the input for the next command
			}
		      	else if (pid == 0){//child
			      	dup2(save, 0); //change the input onto stdin
			      	if (*(command + 1) != NULL){ //if this is not last command
					dup2(fd[1], 1); //set input write pipe to stdout
			      	}
			      	close(fd[0]); //close read pipe
			      	statusArray[(*command)->count] = processPipeCommand(command,originalCommand,i); //process pipe command
			      	//execvp((*cmd)->allCommands[0], (*cmd)->allCommands);
			      	exit(EXIT_FAILURE);
			 
			}
			else{
				fprintf(stderr,"Error: fork error");
			}
		}
		printf("Pipe Return Status: %d\n",statusArray[1]);
		//fprintf(stderr,"+ completed '%s' [%d]\n",pipes->originalUserInput,0);
		fprintf(stderr, "+ completed '%s' ",pipes->originalUserInput);
		for( i = 0 ; i <pipes->argumentCount;i++){
			if(statusArray[i] != 0){
				statusArray[i] = statusArray[i]/256;
			}
			fprintf(stderr, "[%d]",statusArray[i]);
		}
		fprintf(stderr,"\n");
	}
}

int processPipeCommand(Command **command, char **originalCommand, const int count){
	int status;
	if(*(command+1) == NULL){ //this is last command
		if((*command)->outputRedirection){ // if their exist ouput redirection in the last command
			processOutputRedirection((*command),(*originalCommand),(*command)->allCommands[0],true);
		}
		else{
			status = execvp((*command)->allCommands[0], (*command)->allCommands);
		}
	}
	if((*command)->count == 0){ //if this is the first command
		if((*command)->inputRedirection){ // if their exist input redirection in the first command
			processInputRedirection((*command),(*originalCommand),(*command)->allCommands[0],true);
		}
		else{
			status = execvp((*command)->allCommands[0], (*command)->allCommands);
		}
	}
	if (*(command+1) != NULL && count != 0){
		execvp((*command)->allCommands[0], (*command)->allCommands);
	}
	int es = WEXITSTATUS(status);
	return es;
}

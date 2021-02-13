#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_LEN 1024

size_t read_command(char *cmd) {
	if(!fgets(cmd, BUFFER_LEN, stdin)) /*get command and put it in line*/
		return 0;

	/*if user hits CTRL+D break*/
	size_t length = strlen(cmd); /*get command length*/

	if (cmd[length - 1] == '\n') cmd[length - 1] = '\0'; /*clear new line*/

	return strlen(cmd); /*return length of the command read*/
}

int build_args(char * cmd, char ** argv) {
	char *token; /*split command into separate strings*/
	token = strtok(cmd," ");

	int i=0;

	while(token!=NULL){/*loop for all tokens*/
		argv[i]=token; /*store token*/
		token = strtok(NULL," "); /*get next token*/
		i++; /*increment number of tokens*/
	}

	argv[i]=NULL; /*set last value to NULL for execvp*/
	return i; /*return number of tokens*/
}

void set_program_path (char * path, char * bin, char * prog) {
	memset (path,0,1024); /*intialize buffer*/
	strcpy(path, bin);
	/*copy /bin/ to file path*/
	strcat(path, prog);
	/*add program to path*/
	int i=0;
	for(i=0; i<strlen(path); i++) /*delete newline*/
	if(path[i]=='\n') path[i]='\0';
}

/*int commandType (char* cmd, )*/



int main(){

	char line [BUFFER_LEN];/*get command line*/
	char* argv[100];/*user command*/
	char* bin="/bin/";/*set path at bin*/
	char path[1024];/*full file path*/
	int argc;/*arg count*/

	while (1) {

		printf("My shell>> ");/*print shell prompt*/

		if (read_command(line) == 0 )
			{printf("\n"); break;} /*CRTL+D pressed*/
		
		if (strcmp(line, "exit") == 0) break; /*exit*/
		
		argc = build_args (line,argv); /*build program argument*/
		set_program_path (path,bin,argv[0]); /*set program full path*/
		
		int pid= fork(); /*fork child*/
		
		if(pid==0){      /*Child*/
			execve(path,argv,0); /*if failed process is not replaced*/
			/*then print error message*/
			
			if (strcmp(argv[0], "cd") == 0) {
				chdir(argv[1]);
			} else if (strcmp(argv[0], "hello") == 0) {
				char *name = getenv("USER");
				printf("%s\n", name);
			}

			fprintf(stderr, "Child process could not do execve\n");
		
		} else wait(NULL); /*Parent*/
	}

	return 0;
}
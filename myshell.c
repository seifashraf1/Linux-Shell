#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <ctype.h>
#include <glob.h>

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

/*this functions takes the command and returns*/
/*2: if input redirection*/
/*3: if output redirection*/
/*1: if both*/
/*4: if pipes*/
/*5: if vars set*/
/*6: if neither (Simple command)*/
int commandType (char* cmd) {
	char *i = strchr(cmd, '<');
	char *o = strchr(cmd, '>');
	char *p = strchr(cmd, '|');
	char *v = strchr(cmd, '=');

	if ((i != NULL || o != NULL) && (p != NULL)) return 1;
	else if (i != NULL || o != NULL) return 2; /*redirection*/
	else if (p != NULL) return 3; /*pipelining*/
	/*else if ((i != NULL || o != NULL) && (p != NULL)) return 4;*/
	else if (v != NULL) return 5;
	else return 6;		/*neither*/ 
}

void handleRedirection (char *filename, int input) {

	if (input) {
		int fd;
		if((fd = open(filename, O_RDONLY, 0644)) < 0){
			perror("open error");
			return;
		}

		dup2(fd, 0);
		close(fd);

	} else  {
		int fd;
		if((fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0644)) < 0){
			perror("open error");
			return;
		}
		
		dup2(fd, 1);
		close(fd);
	}

}

void handleMultiRedirections (char* inputfile, char* outputfile) {

		int fd;
		inputfile[strlen(inputfile)-1] = '\0';
		
		if((fd = open(inputfile, O_RDONLY, 0644)) < 0){
			printf("fd:%d\n", fd);
			perror("open error");
			return;
		}

		dup2(fd, 0);
		close(fd);

		int fd2;
		if((fd2 = open(outputfile, O_CREAT | O_WRONLY | O_TRUNC, 0644)) < 0){
			printf("fd2:%d\n", fd2);
			perror("open error");
			return;
		}
		
		dup2(fd2, 1);
		close(fd2);


}

int countCMD (char* cmd, char** simpleCommands){
    char* command = strtok(cmd, "|");
    int i=0;
    while(command!=NULL){
        simpleCommands[i]=command;
        command = strtok(NULL,"|"); 
        i++; 
    }
    simpleCommands[i]=NULL; 
    return i; 
}

void handleMultiPipes (char* line) {
	int stdin = dup(0);
	int stdout = dup(1);

	char* argv[100];/*user command*/
	char* bin="/bin/";/*set path at bin*/
	char path[1024];/*full file path*/
	int argc;/*arg count*/

	char lineCP [BUFFER_LEN];		/*to get simple commands*/
	strcpy(lineCP, line);

	char iline [BUFFER_LEN];		/*to parse input file*/
	strcpy(iline, line);

	char oline [BUFFER_LEN];		/*to parse output file*/
	strcpy(oline, line);
	
	argc = build_args (line,argv); /*build program argument*/
	set_program_path (path,bin,argv[0]); /*set program full path*/


	char* simpleCommands[100];
	int cmdNum = 0;
	cmdNum = countCMD(lineCP, simpleCommands);
	printf("num:%d\n", cmdNum);

	/*parsing input files if any, and redirects*/
	char *iflg = strchr(iline, '<');
	int fdi;

	if (iflg != NULL) {
		char* inputfile = strtok(iflg+1, ">");
		memmove(inputfile, inputfile+1, strlen(inputfile));
		inputfile[strlen(inputfile)-1]='\0';
		
		if((fdi = open(inputfile, O_RDONLY, 0644)) < 0){
			perror("open error");
			return;
		}
			
	} else {
		fdi=dup(stdin);
	}

	
	int pid;
	int fdo;
	int i=0;


	for (i=0; i<cmdNum; i++) {
		char* argv2[100];/*user command*/
		char* bin2="/bin/";/*set path at bin*/
		char path2[1024];/*full file path*/
		int argc2;/*arg count*/
		
		char lineCP2 [BUFFER_LEN];
		strcpy(lineCP2, simpleCommands[i]);
		
		argc = build_args (simpleCommands[i],argv2); /*build program argument*/
		set_program_path (path2,bin2,argv2[0]);

		dup2(fdi, 0);
		close(fdi);
		
		printf("%s\n", oline);
		printf("%s\n", simpleCommands[i]);

	if (i==cmdNum-1) {
		char *oflg = strchr(oline, '>');

		if (oflg != NULL) {
			char* outputfile = strtok(oflg+1, " ");
			printf("%s\n", outputfile);
			
			if((fdo = open(outputfile, O_CREAT | O_WRONLY | O_TRUNC, 0644)) < 0){
				perror("open error");
				return;
			}
		
		} else {
			fdo=dup(stdout);
		}

	} else {

		int fdPipe[2];
		pipe(fdPipe);

		fdo=fdPipe[1];
		fdi=fdPipe[0];
	} 

		dup2(fdo, 1);
		close(fdo);

		pid=fork();


		if (pid==0) {
			/*int input = 1;

			if (strcmp(argv2[1], ">") == 0) input = 0;

			if (input) {
				char *cmd = strtok(lineCP2, "<");
				char *filename = strtok (NULL, " ");
				/*handleRedirection(filename, input);
				argv2[1] = NULL; 

			} else if (!input) {
				char *cmd = strtok(lineCP2, ">");
				char *filename = strtok (NULL, " ");
				/*handleRedirection(filename, input);
				argv2[1] = NULL;

			} 	*/	
			if (strcmp(argv2[1], ">") == 0 || strcmp(argv2[1], "<") == 0) {
				argv2[1] = NULL;
			}
			execve(path2, argv2, 0);
		}

	}

		dup2(stdin, 0);
		dup2(stdout, 1);
		close(stdin);
		close(stdout);

		wait(NULL);
	
}

void handlePipes (char* cmd1, char* cmd2, char** argv) {
	int fd[2];
	if(pipe(fd)==-1) {
		printf("ERROR: Pipe cannot be created!\n");
		return;
	}

	int pid1 = fork();

	if (pid1 == 0) {
		/*child process 1 (cmd 1)*/
		dup2(fd[1], 1);
		close(fd[1]);
		char* argv[100];/*user command*/
		char* bin="/bin/";/*set path at bin*/
		char path[1024];/*full file path*/
		int argc;/*arg count*/
		argc = build_args (cmd1,argv); /*build program argument*/
		set_program_path (path,bin,argv[0]); /*set program full path*/
		execve(path, argv, 0);
	} else if (pid1 < 0) {
		printf("ERROR: process 1 failed!\n");
	}

	int pid2 = fork();

	if (pid2 == 0) {
		/*child process 2 (cmd 2)*/
		dup2(fd[0], 0);
		close(fd[0]);
		char* argv[100];/*user command*/
		char* bin="/bin/";/*set path at bin*/
		char path[1024];/*full file path*/
		int argc;/*arg count*/
		argc = build_args (cmd2,argv); /*build program argument*/
		set_program_path (path,bin,argv[0]); /*set program full path*/
		execve(path, argv, 0);
	} else if (pid2 < 0) {
		printf("ERROR: process 2 failed!\n");
	}

	wait(NULL);

	
}

void handleTicks (char* var, char* cmd) {
	int fd[2];
	if(pipe(fd)==-1) {
		printf("ERROR: Pipe cannot be created!\n");
		return;
	}

	int pid = fork();

	if (pid == 0) {
		/*child process 1 (cmd 1)*/
		dup2(fd[1], 1);
		close(fd[1]);
		char* argv[100];/*user command*/
		char* bin="/bin/";/*set path at bin*/
		char path[1024];/*full file path*/
		int argc;/*arg count*/
		argc = build_args (cmd,argv); /*build program argument*/
		set_program_path (path,bin,argv[0]); /*set program full path*/
		execve(path, argv, 0);
	} else if (pid < 0) {
		printf("ERROR: process 1 failed!\n");
	}

	char buffer [BUFFER_LEN] = {0};

	read (fd[0], buffer, BUFFER_LEN);

	setenv(var, buffer, 1);

}


int main(){

	char line [BUFFER_LEN];/*get command line*/
	char* argv[100];/*user command*/
	char* bin="/bin/";/*set path at bin*/
	char path[1024];/*full file path*/
	int argc;/*arg count*/
	char lineCP [BUFFER_LEN];

	while (1) {

		printf("My shell>> ");/*print shell prompt*/

		if (read_command(line) == 0 )
			{printf("\n"); break;} /*CRTL+D pressed*/
		
		if (strcmp(line, "exit") == 0) break; /*exit*/
		
		int type = commandType(line);
		strcpy(lineCP, line);
		
		argc = build_args (line,argv); /*build program argument*/
		set_program_path (path,bin,argv[0]); /*set program full path*/

		if (strcmp(argv[0], "cd") == 0) {	/*cd*/
				chdir(argv[1]);
				continue;
		} 

		if (strcmp(argv[0], "whoami") == 0) {
				char *name = getenv("USER");
				printf("%s\n", name);
				continue;
		}

		if (strchr(argv[argc-1], '*') != NULL) { 
			glob_t buffer;
			const char * pattern = argv[argc-1];
			int i;
			int files_count; 

			glob( pattern , 0 , NULL , &buffer ); 

			files_count = buffer.gl_pathc;

			for (i=0; i < files_count; i++) {
				int glob_pid= fork();
				
				if(glob_pid==0){
					argv[argc-1] = buffer.gl_pathv[i];
					execvp(argv[0], argv); 
			    	/*printf("%s \n",buffer.gl_pathv[i]);*/
			   		globfree(&buffer);
				
			} else wait(NULL);

		}

			
			continue;

		}
		
		if (strcmp(argv[0], "echo") == 0) { 
			char *var = argv[1]; 
			char *v = getenv(var+1); 
			printf("%s\n", v); 
			continue;


		}

		if (type == 5) {

			char* ftick = strchr(lineCP, '`');
			char* ltick = strrchr(lineCP, '`');

			if (ftick != NULL && ltick != NULL) {
				char *var = strtok(lineCP, "=");
				
				char* cmd = strtok(NULL, "`");
				
				handleTicks(var, cmd);

			} else {
				char *var = strtok(argv[0], "=");
				char *val = strtok(NULL, " ");
				
				char *var2 = strchr(val, '$');

				if (var2 != NULL){
					memmove(val, val+1, strlen(val));
					/*char* tmp = strtok(NULL, " ");
					char* val2 = strtok(NULL, " ");
					char *val3 = getenv(val2); */
					char *v = getenv(val); 
					setenv(var, v, 1);
				} else {
					setenv(var, val, 1);
				}
				
			}
			
			
			continue;
		}
		
		printf("%d\n", type);

		
		int pid= fork(); /*fork child*/
		
		if(pid==0){      /*Child*/

			if (type == 1) {
				handleMultiPipes(lineCP);
				continue;
				
			} else if (type == 2) {
				int input = 1;

				char* check = strchr(lineCP, '>');
				if (check != NULL) input = 0;

				if (input) {
					char *cmd = strtok(lineCP, "<"); 
				} else {
					char *cmd = strtok(lineCP, ">");
				}
				char *filename = strtok (NULL, " ");

				handleRedirection(filename, input);
				argv[argc-2] = NULL;
				
			} else if (type == 3) {
				char* cmd1 = strtok(lineCP, "|");
				char* cmd2 = strtok(NULL, "\0");

				/*memmove(cmd2, cmd2+1, strlen(cmd2));*/

				handlePipes(cmd1, cmd2, argv);
				continue;
				
			} else if (type==10) {
				char *cmd = strtok(lineCP, "<"); 
				char *inputfile = strtok (NULL, ">");
				char *outputfile = strtok (NULL, " ");
				memmove(inputfile, inputfile+1, strlen(inputfile));
				handleMultiRedirections(inputfile, outputfile);
				argv[1] = NULL;
				/*execvp(argv[0], argv);
				continue;*/

			}

			execve(path,argv,0); /*if failed process is not replaced then print error message*/
			fprintf(stderr, "Child process could not do execve\n");
		
		} else wait(NULL); /*Parent waits for child to die*/
	}

	return 0;
}
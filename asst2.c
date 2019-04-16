#include <unistd.h>
#include <stdlib.h>     
#include <stdio.h>    
#include <sys/wait.h>	
#include <string.h>  
#include <errno.h>  
#include <fcntl.h>    

int terminal;
int status;
int bg=0,i,m,k=0;
int fdp[100][20];
char *bg_proc[100];
pid_t pid,gpid,pgid,fpid,kpid,cpid;
char *arg[100];
char *arr1[100];
char *arr2[100];
int get(char* cmd, char** arg, char** arr1,char** arr2);
int run(char** arg,int num, char** arr1,char** arr2);
int num=0;
void handle_interrupt(int signum)
{
	if(signum==SIGCHLD)		//sighandler for sigchld
	{
		kill(pid, SIGKILL);
		waitpid(-1, NULL, WNOHANG);	
		fflush(stdout); 
	}
	else if(signum==SIGINT)	//sighandler for sigint ie cntr c
	{		
		printf("\n");
		printf("minish> ");
		fflush(stdout); 
	}
}

void handler(int signal)
{
	if(waitpid(-1,NULL,WNOHANG)==-1)
	{
		perror("wait failed");
		exit(1);
	}
	if(!tcsetpgrp(terminal,pgid))
		perror("tcsetpgrp failed");
}

void handle_suspend()
{
	if(tcsetpgrp(terminal,pgid)==-1)
		perror("tcsetpgrp failed");
}

int main()
{
	
	gpid=getpid();
	//signal(SIGCHLD, handler);
	//setpgid(gpid,gpid);
	terminal = STDIN_FILENO;
	char cmd[100];
	char check;
	pgid=getpgrp();
	signal(SIGINT, handle_interrupt);
	signal(SIGCHLD, handle_interrupt);

	tcsetpgrp(terminal, pgid);
	while (1)
	{	
		bg=0;
        printf("minish> ");

		if(fgets(cmd, sizeof(cmd), stdin)==0)
		{
			fprintf(stderr,"Failed while reading input");
			break;
		}
		int r=strlen(cmd)-1;
		cmd[r] = '\0'; //remove the extra characters
        
		num=get(cmd,arg,arr1,arr2);
		if(strcmp(arg[0],"exit")==0)
		{
			printf("\n Exitting the shell \n");
			waitpid(-1, NULL, WNOHANG);
			 break;
		}		
		run(arg,num,arr1,arr2);
		//run(arg,num);
	}
    return 0;
}


//separate the command to array
int get(char* cmd, char** arg, char** arr1,char** arr2)
{      
	int i,countpipe;
	int y=0;
    for(i = 0; i < 100; i++) 
	{
        arg[i] = strsep(&cmd, " ");
        if(arg[i] == NULL) 
		{
			return i;//total no of arg
		}
    }
}


int run(char** arg, int num, char** arr1,char** arr2)
{
	char** pipearray[100][100];
	if(strcmp(arg[num-1],"&")==0)
	{
		bg=1;
	}    
	// Forking process
    pid = fork();
	fpid=pid;
	
    if(pid<0)
    {
        printf("Fork Error");
        exit(1);
    }
    
	else if (pid == 0)  // If child process
	{
		cpid=getpid();
		//----------------CHANGE DIRECTORY---------------------------
		if(strcmp(arg[0],"cd")==0)
		{
			if(arg[1] == NULL)
				chdir(getenv("HOME"));
			else if (chdir(arg[1]) == -1) 
				printf(" %s: no such directory\n", arg[1]);
			return 1;
		}
		//---------------KILL----------------------------
		if(strcmp(arg[0],"kill")==0)
		{
			kpid=atoi(arg[1]);
			//printf("%d pid id ",kpid);
			//signal(SIGCHLD, handle_interrupt);
			if(kill(kpid, SIGKILL)==-1)
				perror("Kill error");
			return 1;	
		}
		
		//----------------------I/O REDIRECTION---------------------
		int i=0,fd;
		for(i=0;i<num;i++)
		{
			if(strcmp(arg[i],"|")==0)
			{
				int z=0;	
				int y=0;
				for(int p=0; p< num;p++)
				{
					int a=0,q;
					if(strcmp(arg[p],"|")==0)
					{
						for( q=0; q< p;q++)
						{ 
							memcpy(&arr1[q],&arg[q],sizeof(char *));		//copy left part of pipe 		
						}
						
						for(int r=p+1; r<=num;r++)
						{
							memcpy(&arr2[y],&arg[r],sizeof(char *));		//copy right part of pipe 
							y++;
						}										
					}
				}
				
				for(int p=0; p< num;p++)
				{
					int fds[2];
					if(pipe(fds) == -1) 
					{
						perror("pipe");
						exit(1);
					}
					pid_t ppid = fork();
					if (ppid == -1) 
					{ //error
						printf("error fork!!\n");
						return 1;
					}
					if(ppid==0) // first
					{			
							close(1);//close stdout
							dup(fds[1]);//duplicate write end
							close(fds[0]);//close read end
							close(fds[1]);
							if(execvp(arr1[0], arr1)==-1);  
							{
								printf("EXEC ERROR\n");      
								exit(1);
							}
					}
					else if(ppid>0) // last
					{	int k=y-1;
						
						close(0);//close stdin
						dup(fds[0]);//duplicate read end
						close(fds[0]);//close 
						close(fds[1]);//close write end
						for(int h=0;h<k;h++)//check for io redirection for last command
						{
							if(strcmp(arr2[h],"<")==0)
							{
								if (arr2[h+1] == NULL)
								{
									printf("Not enough arguments\n");
								}
								arr2[h] = NULL;
								fd = open(arr2[h+1], O_RDONLY);  
								if(dup2(fd,0)<0)
									perror("Dup error");
								close(fd);
								
							}
							if(strcmp(arr2[h],">")==0)
							{
								if (arr2[h+1] == NULL)
								{
									printf("Not enough arguments\n");
								}
								arr2[h] = NULL;
								fd = open(arr2[h+1], O_CREAT | O_TRUNC | O_WRONLY,0600); 
								if(dup2(fd,1)<0)
									perror("Dup error");
								close(fd);
							}
						}
						if(execvp(arr2[0], arr2)==-1);  
						{
							printf("EXEC ERROR\n");      
							exit(1);
						}
					}					
				}
				
			}
			else if(strcmp(arg[i],"<")==0)
			{
				if (arg[i+1] == NULL)
				{
                    printf("Not enough arguments\n");
				}
				arg[i] = NULL;
				fd = open(arg[i+1], O_RDONLY);  
				if(dup2(fd,0)<0)
					perror("Dup error");
				close(fd);
				
			}
			else if(strcmp(arg[i],">")==0)
			{
				if (arg[i+1] == NULL)
				{
                    printf("Not enough arguments\n");
				}
				arg[i] = NULL;
				fd = open(arg[i+1], O_CREAT | O_TRUNC | O_WRONLY,0600); 
				if(dup2(fd,1)<0)
					perror("Dup error");
				close(fd);
			}
			
		}
		//-------------------------------------------
		if(bg==1)
		{
			bg=1;
			arg[num-1] = NULL;
			setpgid(0,0);
			printf( " Process %d  in background mode \n",getpid());
		}
		//-------------------------------------------

		if(execvp(arg[0], arg)==-1);  
		{
			printf("EXEC ERROR\n");      
			exit(1);
		}
		exit(0);	
	}
	
    else 
	{ // Parent process

        if(bg==0)
		{
			int childStatus; // Wait for child process to finish
			if(waitpid(pid, &childStatus, WUNTRACED)==-1)
			{
				perror("wait failed");
				exit(1);
			}
			return 1;
		}
		if(bg==1)
		{/*
			int childStatus; // Wait for child process to finish
			if(waitpid(pid, &childStatus, WNOHANG)==-1)
			{
				perror("wait failed");
				exit(1);
			}
			return 1;
*/
		}
    }
}

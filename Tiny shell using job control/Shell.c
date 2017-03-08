#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "header.h"

//Main Function
int main(int argc, char **argv) 
{
    char commandline[maxline_no];
    int initiate_prompt = 1; // initiate prompt (default)

    Signal(SIGINT,  SignalINT);   // ctrl-c pressing
    Signal(SIGTSTP, SignalStop);  // ctrl-z pressing
    Signal(SIGCHLD, SignalChild);  // terminated child process

    initiate_job(jobs); //initiate job

    while (1) //runs forever untill exit command not executed
    { 
	if (initiate_prompt) //always true
	{
	    printf("%s", prompt);                   
	    fflush(stdout);
	}

	if ((fgets(commandline, maxline_no, stdin) == NULL) && ferror(stdin)) //if we didn't press any key and some standered input error occure
	    error_msg("Standered input error");
	
	// Evaluate the command line 
	evaluation(commandline);//evaluation of commandline
	fflush(stdout);
	fflush(stderr);
    } 
} // END OF MAIN FUNCTION
  // evaluation - Evaluate the command line that the user has just typed in  
void evaluation(char *commandline) 
{
	char *argv[maxarg]; 		
	int bg;				 // should the job run in bg or fg? 
	pid_t pid;			 // process id 
	struct job_t *jobdetails;

	bg = parseline(commandline, argv);	//checking command is in fg or bg
	if (argv[0] == NULL)
		return; // ignore empty lines 

	if (!inbuilt_command(argv))	//if command is not built-in than
	{
		if ((pid = Fork()) == 0) // creating child process
		{  
			setpgid(0,0); //setting process id==0,process group id ==0        
			if (execve(argv[0], argv, environ) < 0) 
			{
				printf("%s: Command not found. Please try again\n", argv[0]);//chaking command by using default environ variable, if it return -1 than command is not found
				exit(0);
			}
		}

		// parent waits for foreground job to terminate 
		if (!bg) //if process is foreground
		{
			add_job(jobs,pid,FG,commandline);	//adding job in FG
			waitfg_terminate(pid);//Condition Variable (if any process currently running than wait for process to terminate)
			jobdetails=findjob_pid(jobs,pid);//finding job details
			if(jobdetails!=NULL && (*jobdetails).state!=ST)
			{
				kill(pid, SIGKILL);
				delete_job(jobs,pid);}    //delete job after it's terminated
			}
		else // if process is in background
		{
			add_job(jobs,pid,BG,commandline);	// adding job in BG
			jobdetails = findjob_pid(jobs, pid);//finding job details
			printf("Jobid-->[%d], Processid-->(%d) Job-->%s",(*jobdetails).jid,(*jobdetails).pid,(*jobdetails).commandline);//printing details on screen
		}
	}
}// END OF evaluation FUNCTION


// add_job - Add a job to the job list 
int add_job(struct job_t *jobs, pid_t pid, int state, char *commandline) 
{
    int i;
    
    if (pid < 1)
	return 0;

    for (i = 0; i < maxjob; i++) {
	if (jobs[i].pid == 0) {
	    jobs[i].pid = pid;
	    jobs[i].state = state;
	    jobs[i].jid = next_job_id++;
	    if (next_job_id > maxjob)
		next_job_id = 1;
	    strcpy(jobs[i].commandline, commandline);
	    return 1;
	}
    }
    printf("Too many jobs created\n");
    return 0;
}

// delete_job - Delete a job whose PID=pid from the job list
int delete_job(struct job_t *jobs, pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;

    for (i = 0; i < maxjob; i++) {
	if (jobs[i].pid == pid) {
	    clearjob(&jobs[i]);
	    next_job_id = maxjid(jobs)+1;
	    return 1;
	}
    }
    return 0;
}


//In this function will evaluate how many argument present in commandline and also deside command is written in bg or fg
int parseline(const char *commandline, char **argv) 
{
    static char array[maxline_no]; // command line store into array
    char *buffer = array;          // pointer to traverses command line 
    char *delimiter;               // points to first space delimiter 
    int argc;                   // number of args 
    int bg;                     // background job? 

    strcpy(buffer, commandline);//copy commandline into buffer

    buffer[strlen(buffer)-1] = ' ';  // replace trailing '\n' with space 

    while (*buffer && (*buffer == ' ')) // ignore leading spaces 
	buffer++;
	
    // now  we want to count which argument is present in commandline and how many
    // Build the argv list 
    argc = 0;	//initiate argc with 0

    while ((delimiter = strchr(buffer, ' ')))	
    {
	argv[argc++] = buffer;
	*delimiter = '\0';
	buffer = delimiter + 1;
	while (*buffer && (*buffer == ' ')) // ignore internal spaces 
	       buffer++;
    }
    argv[argc] = NULL;  
    
    if (argc == 0)  // ignore blank line 
	return 1;

    // should the job run in the background? 
    if ((bg = (*argv[argc-1] == '&')) != 0)
	argv[--argc] = NULL;

    return bg;	//returning 1 or 0
}

//waitfg_terminate - Block until process pid is no longer the foreground process
void waitfg_terminate(pid_t pid)
{
    int status;
	if (waitpid(-1,&status,0) < 0)               //wait for pid to 
		unix_error("waitfg_terminate: waitpd error");      //no longer be in FG
}

// clearjob - Clear the entries in a job struct 
void clearjob(struct job_t *job) 
{
    (*job).pid = 0;
    (*job).jid = 0;
    (*job).state = undefined;
    (*job).commandline[0] = '\0';
}

// initiate_job - Initialize the job list 
void initiate_job(struct job_t *jobs) {
    int i;

    for (i = 0; i < maxjob; i++)
	clearjob(&jobs[i]);
}

// maxjid - Returns largest allocated job ID
int maxjid(struct job_t *jobs) 
{
    int i, max=0;

    for (i = 0; i < maxjob; i++)
	if (jobs[i].jid > max)
	    max = jobs[i].jid;
    return max;
}


//inbuilt_command - If the user has typed a built-in command then execute it immediately.  
int inbuilt_command(char **argv) 
{
    if(!strcmp(argv[0], "exit"))  // exit command 
		exit(0);
	if(!strcmp(argv[0], "jobs")){ // jobs command 
		joblist(jobs);
		return 1;}
	if(!strcmp(argv[0], "bg")){ // bg command 
		bg_to_fg(argv);
		return 1;}
	if(!strcmp(argv[0], "fg")){ // fg command 
		bg_to_fg(argv);
		return 1;}
	if(!strcmp(argv[0], "&"))// ignore single & 
		return 1;
	
	return 0;     // not a builtin command 
}

//bg_to_fg - Execute the builtin bg commands (Simply converting bg <--> fg)
void bg_to_fg(char **argv) 
{
	struct job_t *jobdetails;
	int temp;

	if(!strcmp(argv[0], "bg"))//bg command
	{    
		if(argv[1][0] == '%')
		{
			temp=argv[1][1];
			jobdetails=findjob_jid(jobs, temp-48);  //get job id based on %input
			if(jobdetails==NULL)
				printf("%s: No such job\n",argv[1]);
		}

		else
		{
			temp=atoi(argv[1]);
			jobdetails=findjob_pid(jobs, temp);   //get job id based on input
			if(jobdetails==NULL)
				printf("(%d): No such process\n",temp);
		}
	
		printf("[%d] (%d) %s",(*jobdetails).jid,(*jobdetails).pid,(*jobdetails).commandline);
		kill((*jobdetails).pid,SIGCONT);  //send SIGCONT signal to process
		(*jobdetails).state=BG;
	}

	if(!strcmp(argv[0], "fg"))//fg command
	{    
		if(argv[1][0] == '%')
		{
			temp=argv[1][1];
			jobdetails=findjob_jid(jobs, temp-48);   //get job id based on %input
			if(jobdetails==NULL)
				printf("%s: No such job available\n",argv[1]);
		}
		else
		{
			temp=atoi(argv[1]);
			jobdetails=findjob_pid(jobs, temp);    //get job id based on input
			if(jobdetails==NULL)
				printf("(%d): No such process\n",temp);}

			if(jobdetails!=NULL){	
			waitfg_terminate((*jobdetails).pid);
			kill((*jobdetails).pid,SIGCONT);    //send sigcont to process
			if((*jobdetails).state!=ST)
			  delete_job(jobs,(*jobdetails).pid);  //delete the job after termination
		}   
	}
	return;
}

// curr_pid - Return PID of current foreground job, 0 if no such job 
pid_t curr_pid(struct job_t *jobs) {
    int i;

    for (i = 0; i < maxjob; i++)
	if (jobs[i].state == FG)
	    return jobs[i].pid;
    return 0;
}

// findjob_pid  - Find a job (by PID) on the job list 
struct job_t *findjob_pid(struct job_t *jobs, pid_t pid) {
    int i;

    if (pid < 1)
	return NULL;
    for (i = 0; i < maxjob; i++)
	if (jobs[i].pid == pid)
	    return &jobs[i];
    return NULL;
}

// findjob_jid  - Find a job (by JID) on the job list 
struct job_t *findjob_jid(struct job_t *jobs, int jid) 
{
    int i;

    if (jid < 1)
	return NULL;
    for (i = 0; i < maxjob; i++)
	if (jobs[i].jid == jid)
	    return &jobs[i];
    return NULL;
}

// joblist - Print the job list 
void joblist(struct job_t *jobs) 
{
    int i;
    
    for (i = 0; i < maxjob; i++) 
    {
	if (jobs[i].pid != 0) 
	{
	    printf("job id-->[%d] process(%d) ", jobs[i].jid, jobs[i].pid);
	    switch (jobs[i].state) {
		case BG: 
		    printf("Running    ");
		    break;
		case FG: 
		    printf("Foreground ");
		    break;
		case ST: 
		    printf("Stopped    ");
		    break;
	    	default:
		    printf("joblist: Internal error: job[%d].state=%d ",i, jobs[i].state);
	    }
	    printf("%s", jobs[i].commandline);
	}
    }
}

pid_t Fork(void)
{
  pid_t pid;

  if((pid=fork())<0)
    unix_error("Fork error");
  return pid;
}

/* 
 * SignalChild - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.  
 */
void SignalChild(int signal) 
{
    return;
}

/* 
 * SignalINT - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.  
 */
void SignalINT(int signal) 
{
	pid_t fpid;
	struct job_t *jobdetails;
	fpid=curr_pid(jobs);
	jobdetails=findjob_pid(jobs, fpid);

	if(fpid!=0)
	{
		printf("Job [0] (%d) terminated by signal: Interrupt\n",fpid);
		kill(fpid, SIGKILL);   //terminate process
		delete_job(jobs,fpid); //delete job from joblist
	}
    return;
}

/*
 * SignalStop - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.  
 */
void SignalStop(int signal) 
{
    pid_t fpid;
	struct job_t *jobdetails;
	fpid=curr_pid(jobs);
	jobdetails=findjob_pid(jobs, fpid);

	if(fpid!=0)
	{
		printf("Job [%d] (%d) stopped by signal: Stopped\n",(*jobdetails).jid,fpid);
		kill(fpid, SIGKILL);  //terminate process
		(*jobdetails).state=ST;        //change status to stopped
	}
    return;
}

handler_t *Signal(int signum, handler_t *handler) 
{
    struct sigaction action, old_action;

    action.sa_handler = handler;  
    sigemptyset(&action.sa_mask); // block sigs of type being handled 
    action.sa_flags = SA_RESTART; // restart syscalls if possible 

    if (sigaction(signum, &action, &old_action) < 0)
	unix_error("Signal error");
    return (old_action.sa_handler);
}

void unix_error(char *msg)
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

//error_msg - application-style error routine
void error_msg(char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

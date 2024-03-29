#include <sys/types.h>
#include <termios.h>
#include <unistd.h> /* getpid()*/
#include <signal.h> /* signal name macros, and sig handlers*/
#include <stdlib.h> /* for exit() */
#include <errno.h> /* for errno */
#include <sys/wait.h> /* for WAIT_ANY */
#include <string.h>

#include "dsh.h"

int isspace(int c);

/* Keep track of attributes of the shell.  */
pid_t shell_pgid;
struct termios shell_tmodes;
int shell_terminal;
int shell_is_interactive;

void init_shell();
void spawn_job(job_t *j, bool fg);
job_t * find_job(pid_t pgid);
int job_is_stopped(job_t *j);
int job_is_completed(job_t *j);
bool free_job(job_t *j);

/* Initializing the header for the job list. The active jobs are linked into a list. */
job_t *first_job = NULL;

/* Find the job with the indicated pgid.  */
job_t *find_job(pid_t pgid) {

	job_t *j;
	for(j = first_job; j; j = j->next)
		if(j->pgid == pgid)
	    		return j;
	return NULL;
}

/* Return true if all processes in the job have stopped or completed.  */
int job_is_stopped(job_t *j) {

	process_t *p;
	for(p = j->first_process; p; p = p->next)
		if(!p->completed && !p->stopped)
	    		return 0;
	return 1;
}

/* Return true if all processes in the job have completed.  */
int job_is_completed(job_t *j) {

	process_t *p;
	for(p = j->first_process; p; p = p->next)
		if(!p->completed)
	    		return 0;
	return 1;
}

/* Find the last job.  */
job_t *find_last_job() {

	job_t *j = first_job;
	if(!j) return NULL;
	while(j->next != NULL)
		j = j->next;
	return j;
}

/* Find the last process in the pipeline (job).  */
process_t *find_last_process(job_t *j) {

	process_t *p = j->first_process;
	if(!p) return NULL;
	while(p->next != NULL)
		p = p->next;
	return p;
}

bool free_job(job_t *j) {
	if(!j)
		return true;
	free(j->commandinfo);
	free(j->ifile);
	free(j->ofile);
	process_t *p;
	for(p = j->first_process; p; p = p->next) {
		int i;
		for(i = 0; i < p->argc; i++)
			free(p->argv[i]);
	}
	free(j);
	return true;
}

/* Make sure the shell is running interactively as the foreground job
 * before proceeding.  
 * */

void init_shell() {

  	/* See if we are running interactively.  */
	shell_terminal = STDIN_FILENO;
	/* isatty test whether a file descriptor referes to a terminal */
	shell_is_interactive = isatty(shell_terminal);

	if(shell_is_interactive) {
    		/* Loop until we are in the foreground.  */
    		while(tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
      			kill(- shell_pgid, SIGTTIN);

    		/* Ignore interactive and job-control signals.  */
                /* If tcsetpgrp() is called by a member of a background process 
                 * group in its session, and the calling process is not blocking or
                 * ignoring  SIGTTOU,  a SIGTTOU signal is sent to all members of 
                 * this background process group.
                 */

		signal(SIGTTOU, SIG_IGN);

		/* Put ourselves in our own process group.  */
		shell_pgid = getpid();
		if(setpgid(shell_pgid, shell_pgid) < 0) {
			perror("Couldn't put the shell in its own process group");
			exit(1);
		}

		/* Grab control of the terminal.  */
		tcsetpgrp(shell_terminal, shell_pgid);

		/* Save default terminal attributes for shell.  */
		tcgetattr(shell_terminal, &shell_tmodes);
	}
}

/* Sends SIGCONT signal to wake up the blocked job */
void continue_job(job_t *j) {
	if(kill(-j->pgid, SIGCONT) < 0)
		perror("kill(SIGCONT)");
}


/* Spawning a process with job control. fg is true if the 
 * newly-created process is to be placed in the foreground. 
 * (This implicitly puts the calling process in the background, 
 * so watch out for tty I/O after doing this.) pgid is -1 to 
 * create a new job, in which case the returned pid is also the 
 * pgid of the new job.  Else pgid specifies an existing job's 
 * pgid: this feature is used to start the second or 
 * subsequent processes in a pipeline.
 * */

void spawn_job(job_t *j, bool fg) {

	pid_t pid;
	process_t *p;

	/* Check for input/output redirection; If present, set the IO descriptors 
	 * to the appropriate files given by the user 
	 */


	/* A job can contain a pipeline; Loop through process and set up pipes accordingly */


	/* For each command (process), fork to create a new process context, 
	 * set the process group, and execute the command 
         */ 	

	/* The code below provides an example on how to set the process context for each command */

	for(p = j->first_process; p; p = p->next) {

		switch (pid = fork()) {

		   case -1: /* fork failure */
			perror("fork");
			exit(EXIT_FAILURE);

		   case 0: /* child */

		       /* establish a new process group, and put the child in
			* foreground if requested
			*/
			if (j->pgid < 0) /* init sets -ve to a new process */
				j->pgid = getpid();
			p->pid = 0;

			if (!setpgid(0,j->pgid))
				if(fg) // If success and fg is set
				     tcsetpgrp(shell_terminal, j->pgid); // assign the terminal

			/* Set the handling for job control signals back to the default. */
			signal(SIGTTOU, SIG_DFL);


			/* execute the command through exec_ call */

		   default: /* parent */
			/* establish child process group here to avoid race
			* conditions. */
			p->pid = pid;
			if (j->pgid <= 0)
				j->pgid = pid;
			setpgid(pid, j->pgid);
		}

		/* Reset file IOs if necessary */

		if(fg){
			/* Wait for the job to complete */
		}
		else {
			/* Background job */
		}
	}
}

bool init_job(job_t *j) {
	j->next = NULL;
	if(!(j->commandinfo = (char *)malloc(sizeof(char)*MAX_LEN_CMDLINE)))
		return false;
	j->first_process = NULL;
	j->pgid = -1; 	/* -1 indicates new spawn new job*/
	j->notified = false;
	j->mystdin = STDIN_FILENO; 	/* 0 */
	j->mystdout = STDOUT_FILENO;	/* 1 */ 
	j->mystderr = STDERR_FILENO;	/* 2 */
	j->bg = false;
	j->ifile = NULL;
	j->ofile = NULL;
	return true;
}

bool init_process(process_t *p) {
	p->pid = -1; /* -1 indicates new process */
	p->completed = false;
	p->stopped = false;
	p->status = -1; /* set by waitpid */
	p->argc = 0;
	p->next = NULL;
	
        if(!(p->argv = (char **)calloc(MAX_ARGS,sizeof(char *))))
                return false;

	return true;
}

bool readprocessinfo(process_t *p, char *cmd) {

	int cmd_pos = 0; /*iterator for command; */
	int args_pos = 0; /* iterator for arguments*/

	int argc = 0;
	
	while (isspace(cmd[cmd_pos])){++cmd_pos;} /* ignore any spaces */
	if(cmd[cmd_pos] == '\0')
		return true;
	
	while(cmd[cmd_pos] != '\0'){
		if(!(p->argv[argc] = (char *)calloc(MAX_LEN_CMDLINE, sizeof(char))))
			return false;
		while(cmd[cmd_pos] != '\0' && !isspace(cmd[cmd_pos])) 
			p->argv[argc][args_pos++] = cmd[cmd_pos++];
		p->argv[argc][args_pos] = '\0';
		args_pos = 0;
		++argc;
		while (isspace(cmd[cmd_pos])){++cmd_pos;} /* ignore any spaces */
	}
	p->argv[argc] = NULL; /* required for exec_() calls */
	p->argc = argc;
	return true;
}

bool invokefree(job_t *j, char *msg){
	fprintf(stderr, "%s\n",msg);
	return free_job(j);
}

/* Prints the active jobs in the list.  */
void print_job() {
	job_t *j;
	process_t *p;
	for(j = first_job; j; j = j->next) {
		fprintf(stdout, "job: %s\n", j->commandinfo);
		for(p = j->first_process; p; p = p->next) {
			fprintf(stdout,"cmd: %s\t", p->argv[0]);
			int i;
			for(i = 1; i < p->argc; i++) 
				fprintf(stdout, "%s ", p->argv[i]);
			fprintf(stdout, "\n");
		}
		if(j->bg) fprintf(stdout, "Background job\n");	
		else fprintf(stdout, "Foreground job\n");	
		if(j->mystdin == INPUT_FD)
			fprintf(stdout, "Input file name: %s\n", j->ifile);
		if(j->mystdout == OUTPUT_FD)
			fprintf(stdout, "Output file name: %s\n", j->ofile);
	}
}

/* Basic parser that fills the data structures job_t and process_t defined in
 * dsh.h. We tried to make the parser flexible but it is not tested
 * with arbitrary inputs. Be prepared to hack it for the features
 * you may require. The more complicated cases such as parenthesis
 * and grouping are not supported. If the parser found some error, it
 * will always return NULL. 
 *
 * The parser supports these symbols: <, >, |, &, ;
 */

bool readcmdline(char *msg) {

	fprintf(stdout, "%s", msg);

	char *cmdline = (char *)calloc(MAX_LEN_CMDLINE, sizeof(char));
	if(!cmdline)
		return invokefree(NULL, "malloc: no space");
	fgets(cmdline, MAX_LEN_CMDLINE, stdin);

	/* sequence is true only when the command line contains ; */
	bool sequence = false;
	/* seq_pos is used for storing the command line before ; */
	int seq_pos = 0;

	int cmdline_pos = 0; /*iterator for command line; */

	while(1) {
		job_t *current_job = find_last_job();

		int cmd_pos = 0; /* iterator for a command */
		int iofile_seek = 0; /*iofile_seek for file */
		bool valid_input = true; /* check for valid input */
		bool end_of_input = false; /* check for end of input */

		/* cmdline is NOOP, i.e., just return with spaces */
		while (isspace(cmdline[cmdline_pos])){++cmdline_pos;} /* ignore any spaces */
		if(cmdline[cmdline_pos] == '\n' || cmdline[cmdline_pos] == '\0' || feof(stdin))
			return false;

                /* Check for invalid special symbols (characters) */
                if(cmdline[cmdline_pos] == ';' || cmdline[cmdline_pos] == '&'
                        || cmdline[cmdline_pos] == '<' || cmdline[cmdline_pos] == '>' || cmdline[cmdline_pos] == '|')
                        return false;

		char *cmd = (char *)calloc(MAX_LEN_CMDLINE, sizeof(char));
		if(!cmd)
			return invokefree(NULL,"malloc: no space");

		job_t *newjob = (job_t *)malloc(sizeof(job_t));
		if(!newjob)
			return invokefree(NULL,"malloc: no space");

		if(!first_job)
			first_job = current_job = newjob;
		else {
			current_job->next = newjob;
			current_job = current_job->next;
		}

		if(!init_job(current_job))
			return invokefree(current_job,"init_job: malloc failed");

		process_t *current_process = find_last_process(current_job);

		while(cmdline[cmdline_pos] != '\n' && cmdline[cmdline_pos] != '\0') {

			switch (cmdline[cmdline_pos]) {

			    case '<': /* input redirection */
				current_job->ifile = (char *) calloc(MAX_LEN_FILENAME, sizeof(char));
				if(!current_job->ifile)
					return invokefree(current_job,"malloc: no space");
				++cmdline_pos;
				while (isspace(cmdline[cmdline_pos])){++cmdline_pos;} /* ignore any spaces */
				iofile_seek = 0;
				while(cmdline[cmdline_pos] != '\0' && !isspace(cmdline[cmdline_pos])){
					if(MAX_LEN_FILENAME == iofile_seek)
						return invokefree(current_job,"input redirection: file length exceeded");
					current_job->ifile[iofile_seek++] = cmdline[cmdline_pos++];
				}
				current_job->ifile[iofile_seek] = '\0';
				current_job->mystdin = INPUT_FD;
				while(isspace(cmdline[cmdline_pos])) {
					if(cmdline[cmdline_pos] == '\n')
						break;
					++cmdline_pos;
				}
				valid_input = false;
				break;
			
			    case '>': /* output redirection */
				current_job->ofile = (char *) calloc(MAX_LEN_FILENAME, sizeof(char));
				if(!current_job->ofile)
					return invokefree(current_job,"malloc: no space");
				++cmdline_pos;
				while (isspace(cmdline[cmdline_pos])){++cmdline_pos;} /* ignore any spaces */
				iofile_seek = 0;
				while(cmdline[cmdline_pos] != '\0' && !isspace(cmdline[cmdline_pos])){
					if(MAX_LEN_FILENAME == iofile_seek) 
						return invokefree(current_job,"input redirection: file length exceeded");
					current_job->ofile[iofile_seek++] = cmdline[cmdline_pos++];
				}
				current_job->ofile[iofile_seek] = '\0';
				current_job->mystdout = OUTPUT_FD;
				while(isspace(cmdline[cmdline_pos])) {
					if(cmdline[cmdline_pos] == '\n')
						break;
					++cmdline_pos;
				}
				valid_input = false;
				break;

			   case '|': /* pipeline */
				cmd[cmd_pos] = '\0';
				process_t *newprocess = (process_t *)malloc(sizeof(process_t));
				if(!newprocess)
					return invokefree(current_job,"malloc: no space");
				if(!init_process(newprocess))
					return invokefree(current_job,"init_process: failed");
				if(!current_job->first_process)
					current_process = current_job->first_process = newprocess;
				else {
					current_process->next = newprocess;
					current_process = current_process->next;
				}
				if(!readprocessinfo(current_process, cmd))
					return invokefree(current_job,"parse cmd: error");
				++cmdline_pos;
				cmd_pos = 0; /*Reinitialze for new cmd */
				valid_input = true;	
				break;

			   case '&': /* background job */
				current_job->bg = true;
				while (isspace(cmdline[cmdline_pos])){++cmdline_pos;} /* ignore any spaces */
				if(cmdline[cmdline_pos+1] != '\n' && cmdline[cmdline_pos+1] != '\0')
					fprintf(stderr, "reading bg: extra input ignored");
				end_of_input = true;
				break;

			   case ';': /* sequence of jobs*/
				sequence = true;
				strncpy(current_job->commandinfo,cmdline+seq_pos,cmdline_pos-seq_pos);
				seq_pos = cmdline_pos + 1;
				break;	

			   case '#': /* comment */
				end_of_input = true;
				break;

			   default:
				if(!valid_input)
					return invokefree(current_job,"reading cmdline: could not fathom input");
				if(cmd_pos == MAX_LEN_CMDLINE-1)
					return invokefree(current_job,"reading cmdline: length exceeds the max limit");
				cmd[cmd_pos++] = cmdline[cmdline_pos++];
				break;
			}
			if(end_of_input || sequence)
				break;
		}
		cmd[cmd_pos] = '\0';
		process_t *newprocess = (process_t *)malloc(sizeof(process_t));
		if(!newprocess)
			return invokefree(current_job,"malloc: no space");
		if(!init_process(newprocess))
			return invokefree(current_job,"init_process: failed");

		if(!current_job->first_process)
			current_process = current_job->first_process = newprocess;
		else {
			current_process->next = newprocess;
			current_process = current_process->next;
		}
		if(!readprocessinfo(current_process, cmd))
			return invokefree(current_job,"read process info: error");
		if(!sequence) {
			strncpy(current_job->commandinfo,cmdline+seq_pos,cmdline_pos-seq_pos);
			break;
		}
		sequence = false;
		++cmdline_pos;
	}
	return true;
}

/* Build prompt messaage; Change this to include process ID (pid)*/
char* promptmsg() {
        return  "dsh$ ";
}

int main() {

	init_shell();

	while(1) {
		if(!readcmdline(promptmsg())) {
			if (feof(stdin)) { /* End of file (ctrl-d) */
				fflush(stdout);
				printf("\n");
				exit(EXIT_SUCCESS);
                	}
			continue; /* NOOP; user entered return or spaces with return */
		}
		/* Only for debugging purposes and to show parser output */
		print_job();

		/* Your code goes here */
		/* You need to loop through jobs list since a command line can contain ;*/
		/* Check for built-in commands */
		/* If not built-in */
			/* If job j runs in foreground */
			/* spawn_job(j,true) */
			/* else */
			/* spawn_job(j,false) */
	}
}
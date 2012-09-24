#ifndef __DSH_H__         /* check if this header file is already defined elsewhere */
#define __DSH_H__

#include <stdio.h>

/* Max length of input/output file name specified during I/O redirection */
#define MAX_LEN_FILENAME 80

/*Max length of the command line */
#define MAX_LEN_CMDLINE	120

/*file descriptors for input and output; the range of fds are from 0 to 1023;
 * 0, 1, 2 are reserved for stdin, stdout, stderr */
#define INPUT_FD  1000
#define OUTPUT_FD 1001

/* using bool as built-in; char is better in terms of space utilization, but
 * code is not succint */
typedef enum { false, true } bool;

/* A process is a single process.  */
typedef struct process {
        struct process *next;       /* next process in pipeline */
	int argc;		    /* useful for free(ing) argv */
        char **argv;                /* for exec; argv[0] is the path of the executable file; argv[1..] is the list of arguments*/
        pid_t pid;                  /* process ID */
        bool completed;             /* true if process has completed */
        bool stopped;               /* true if process has stopped */
        int status;                 /* reported status value from job control; 0 on success and nonzero otherwise */
} process_t;

/* A job is a process itself or a pipeline of processes.
 * Each job has exactly one process group (pgid) containing all the processes in the job. 
 * Each process group has exactly one process that is its leader.
 */
typedef struct job {
        struct job *next;           /* next active job */
        char *commandinfo;          /* entire command line input given by the user; useful for logging and message display*/
        process_t *first_process;   /* list of processes in this job */
        pid_t pgid;                 /* process group ID */
        bool notified;              /* true if user told about stopped job */
         bool bg;                    /* true when & is issued on the command line */
        char *ifile;                /* stores input file name when < is issued */
        char *ofile;                /* stores output file name when > is issued */
        //struct termios tmodes;      /* saved terminal modes */
        int mystdin;
        int mystdout;
        int mystderr;  /* standard i/o channels */
       
} job_t;

#ifdef NDEBUG
        #define DEBUG(M, ...)
#else
        #define DEBUG(M, ...) fprintf(stderr, "[DEBUG] %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif

#endif /* __DSH_H__*/

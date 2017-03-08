#define maxline_no    1024   // maximum line size 
#define maxarg     128   // maximum args on a command line 
#define maxjob      16   // max jobs we can assign 
#define MAXJID    1<<16   // max job ID 

#define undefined 0 // undefined 
#define FG 1    // running in foreground 
#define BG 2    // running in background 
#define ST 3    // stopp

extern char **environ;        
char prompt[] = "Shell> ";    // command line prompt  
int next_job_id = 1;            // next job ID to allocate  

struct job_t {              // The job structure  
    pid_t pid;              // job ProcessID  
    int jid;                // job ID   
    int state;              // undefined, BG, FG, or ST  
    char commandline[maxline_no];  // command line  
};
struct job_t jobs[maxjob]; // The job list  //concurrent processcing 

void evaluation(char *commandline);
int inbuilt_command(char **argv);
void bg_to_fg(char **argv);
void fg_to_bg(char **argv);  
void waitfg_terminate(pid_t pid);
void SignalChild(int signal);
void SignalStop(int signal);
void SignalINT(int signal);

int parseline(const char *commandline, char **argv); 
void clearjob(struct job_t *job);
void initiate_job(struct job_t *jobs);
int maxjid(struct job_t *jobs); 
int add_job(struct job_t *jobs, pid_t pid, int state, char *commandline);
int delete_job(struct job_t *jobs, pid_t pid); 
pid_t curr_pid(struct job_t *jobs);
struct job_t *findjob_pid(struct job_t *jobs, pid_t pid);
struct job_t *findjob_jid(struct job_t *jobs, int job_id);  
void joblist(struct job_t *jobs);
void unix_error(char *msg);
pid_t Fork(void);
void error_msg(char *msg);
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);





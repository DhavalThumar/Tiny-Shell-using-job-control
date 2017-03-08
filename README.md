# Tiny-Shell-using-job-control
A shell is an interactive command-line interpreter that runs programs on behalf of the user.
A shell repeatedly prints a prompt, waits for a command line on stdin, and then carries out
some action (output)as the contents of the command line.

The first word in the command line is either the name of a built-in command or the pathname 
of an executable file. The remaining words are command-line arguments. If the first word is
a built-in command, the shell immediately executes the command in the current process.
Otherwise, the word is assumed to be the pathname of an executable program. In this case,
the shell forks a child process, then loads and runs the program in the context of the child. 
The child processes created as a result of interpreting a single command line are known 
collectively as a job. In general, a job can consist of multiple child processes connected
by Unix pipes. If the command line ends with an ampersand ”&”, then the job runs in the
background, which means that the shell does not wait for the job to terminate before
printing the prompt and awaiting the next command line. Otherwise, the job runs in the 
foreground, which means that the shell waits for the job to terminate before awaiting
the next command line. Thus, at any point in time, at most one job can be running in the 
foreground. However, an arbitrary number of jobs can run in the background.

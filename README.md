# Unix-Shell-with-History-Feature

A primitive Unix based shell interface with history feature in C that supports up to two seperate commands with arguments, piping, redirection etc., using parent and child processes. This project was a part of the University of Cincinnati graduate operating systems class. For a description of each feature in more detail along with pictures, look at "ProjectReport.pdf".


To compile and run:

	1. have Makefile and UnixShell.c in same folder
	2. type "Make all" into terminal (without "")
	3. type "./osh" into terminal
	
This is a simple unix shell with history feature.
It currently supports:

	1. single commands with arguments
		ex. ls -la
    
	2. 2 seperate commands at once, with arguments, using &&
		ex. ls -l && pwd
    
	3. piping two commands with arguments using |
		ex. cat filename | less
    
	4. a form of redirection with arguments using >>
		ex. ls -a >> filename
		
	5. displays 10 most recent commands used using "history"
	
	6. the command "exit"
	
	7. run the most recent command
	
	8. support the command "cd ...." (since this command will not work currently with execvp)
	
	9. the command "help" to display usefeul information
	
	10. run single commands concurrently using &
		ex. logisim &
		
	11. run the Nth command where N <= 10

Known Bugs:

	1. Incorrectly using cat command, especially with |, may cause shell
	    to behave unpredictably
      
	2. After using a command with &, it runs that command concurrently. However,
	    if you end that process, the shell gets misaligned.

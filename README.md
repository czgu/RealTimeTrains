Currently finished on Train Milestone 1: http://www.cgl.uwaterloo.ca/~wmcowan/teaching/cs452/w16/assignments/tc1.pdf
## Installation
To create the elf file, first clone the repo from git, checkout branch by issuing 
`git checkout -b <branchname> t1`, and then issue 
`make` 
directly in the root directory of the project to make the elf file and copy it to the ftp server.
The executable elf is located at `/u/cs452/tftp/ARM/<username>/train1.elf`. To run it, execute:
`load -b 0x00218000 -h 10.15.167.5 "ARM/<username>/train1.elf"; go`
	
Note: The make command copies the elf file to the user’s (as specified by the command whoami) directory on the ftp server.
	The program displays the time elapsed, the percentage of time the idle task is running for, the state of each switch (curved or straight), a list of the ten most recently triggered sensors, and an input prompt. Once the train is triggered, there will be a line tracing the train location lively below the input prompt line.
	
## The following commands are supported: 
```
tr <train id> <train speed>: Sets train with train id to the specified speed.
rv <train id>		   : Reverses train with train id and sets it to the same speed as before
sw <switch id> <switch direction> : Sets switch with switch id to the specified direction (either C or S, for curved and straight respectively).
q			   : Quits the program
st <train id> <module [1-5]> <sensor> <distance past sensor (mm)>: Stop the train where it will be stopped right at some distance after the given sensor. 
cal <mode> <parameters>: calibrates the train
```

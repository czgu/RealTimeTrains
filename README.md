Currently finished on Kernel 2: http://www.cgl.uwaterloo.ca/~wmcowan/teaching/cs452/w16/assignments/k2.pdf

To create the elf file, first clone the repo from git, checkout branch by issuing 
"git checkout -b <branchname> k2", and then issue "make" directly in the root directory of the project to make the elf file and copy it to the ftp server.
The executable elf is located at /u/cs452/tftp/ARM/<username>/kernel2.elf. To run it, execute:
load -b 0x00218000 -h 10.15.167.5 "ARM/<username>/kernel2.elf"; go
Note: The make command copies the elf file to the user's (as specified by the command whoami) directory on the ftp server.
The first task will start the rock paper scissor server. The user can then create up to 9 rock paper scissor clients. Each client have two questions:
1. [<tid>] Do you want to sign up? (y/n)
    Answer y/Y would let client send a sign up request to server. Otherwise client will exit.
2. [<tid>] What move are you going to make? Press other key to quit\n\r(R:rock, P:paper: S:scissor)
Enter R/P/S would play the corresponding move by sending request to server. Otherwise, the client would quit from server and go back asking 1.


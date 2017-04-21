sshell: sshell.o command.o
	gcc -Wall -Werror -o sshell sshell.o command.o
sshell.o: sshell.c command.h
	gcc -Wall -Werror -c -o sshell.o sshell.c
command.o: command.c command.h
	gcc -Wall -Werror -c -o command.o command.c
REPORT.html:REPORT.md
	pandoc -o README.html README.md
clean:
	rm -f sshell README.html sshell.o command.o

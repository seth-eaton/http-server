#include <sys/socket.h> /* for sockets */
#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdarg.h> /* variadic functions */
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <netdb.h>

#define SERVER_PORT 80

#define MAXLINE 4096

void err_die(const char *fmt, ...);

int main(int argc, char **argv) {
	// local variables
	int 			listenfd, connfd, n;
	struct sockaddr_in 	servaddr;
	uint8_t 		buff[MAXLINE+1];
	uint8_t 		recvline[MAXLINE+1];
	FILE *			fPointer;
	char *			fileName;
	char *			endFileName;

	// create socket
	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		err_die("socet error.");

	// set address
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family		= AF_INET;
	servaddr.sin_addr.s_addr	= htonl(INADDR_ANY);
	servaddr.sin_port		= htons(SERVER_PORT); //server port

	// bind and listen
	if ((bind(listenfd,(struct sockaddr *) &servaddr, sizeof(servaddr))) < 0)
		err_die("bind error.");
	if ((listen(listenfd, 10)) < 0)
		err_die("listen error.");
	
	for ( ; ; ) {
		struct sockaddr_in addr;
		socklen_t addr_len;

		// accept until an incoming connection arrivs
		// return a file descriptor to the connection
		printf("waiting for a connection on port %d\n",SERVER_PORT);
		fflush(stdout);
		connfd = accept(listenfd,(struct sockaddr *) NULL, NULL);

		// zero out buffer to ensure null termination
		memset(recvline, 0, MAXLINE);

		// read client message
		while ((n = read(connfd, recvline, MAXLINE-1)) > 0) {
			fprintf(stdout, "\n%s", recvline);

			if (recvline[n-1] == '\n')
				break;
			memset(recvline, 0, MAXLINE);
		}

		// find fileName	
		fileName = strchr((char *) &recvline, '/');
                endFileName = strstr(fileName, " HTTP");
                memset(endFileName, 0, MAXLINE);

		if (n < 0)
			err_die("read error");

		// compare fileName
		if (!strcmp(fileName, "/"))
			fileName = "/index.html";
		
		// open file
		if ((fPointer = fopen((fileName + sizeof(char)), "r")) == NULL)
			fprintf(stdout, "File open error %s\n", fileName);	
	
		// send HTTP response
		snprintf((char*)buff, sizeof(buff), "HTTP/1.0 200 OK\r\n\r\n");
		write(connfd, (char*)buff, strlen((char *)buff));
		
		while(1 && fPointer) {
			memset(buff, 0, MAXLINE);
			int n = fread(buff, 1, MAXLINE, fPointer);
			
			if (n > 0)
				write(connfd, (char *)buff, n);
			if (n < MAXLINE) {
				if (feof(fPointer)) {
					fprintf(stdout, "End of file\nTransfer complete for %s\n", fileName);
				}
				if (ferror(fPointer))
					fprintf(stdout, "Error reading\n");
				break;
			}
		}		
				
		close(connfd);
	}
}	

void err_die(const char *fmt, ...) {
	int errno_save;
	va_list ap;

	// save errno
	errno_save = errno;

	// print fmt + args to stdout
	va_start(ap, fmt);
	vfprintf(stdout, fmt, ap);
	fprintf(stdout,"\n");
	fflush(stdout);

	//print out error message if errno
	if (errno_save != 0) {
		fprintf(stdout, "(errno = %d) : %s\n", errno_save,
		strerror(errno_save));
		fprintf(stdout,"\n");
		fflush(stdout);
	}
	va_end(ap);

	//die
	exit(1);
}

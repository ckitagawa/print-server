#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>


void error(char *msg)
{
	perror(msg);
	exit(1);
}

void readblock(int sock, char *buffer, int flag)
{
	int n;

	bzero(buffer, 512);
	n = recv(sock, buffer, 510, flag);
	if (n < 0)
		error("ERROR reading from socket");
}

void writeblock(int sock, char *rmsg)
{
	int n;

	n = write(sock, rmsg, strlen(rmsg));
	if (n < 0)
		error("ERROR writing to socket");
}

void recieve_file(int sock)
{
	int n, fsize, sz = 0;
	char buffer[512], fname[512], sysout[550], out[6];
	float progress;

	printf("Connection found waiting for filename.\n");
	readblock(sock, buffer, 0);
	printf("Filename: %s\n", buffer);
	strcpy(fname, buffer);
	//strcpy(fname, "temp.txt");
	writeblock(sock, "Ready to recieve filesize");

	readblock(sock, buffer, 0);
	printf("Filesize: %s\n", buffer);
	fsize = atoi(buffer);
	writeblock(sock, "Ready to recieve file");

	FILE *fp;
	fp = fopen(fname, "wb");
	int i = 0;
	while (1) {

		if (sz + 510 >= fsize) {
			readblock(sock, buffer, 0);
			fwrite(buffer, sizeof(char), strlen(buffer), fp);
			fseek(fp, 0L, SEEK_END);
			sz = ftell(fp);
			if (sz >= fsize){
				break;
			}
			else {
				fwrite("\0", sizeof(char), 1, fp);
			}
			break;
		}
		else {
			//fwrite(buffer, sizeof(char), 510, fp);
			readblock(sock, buffer, 0);
			fwrite(buffer, sizeof(char), 510, fp);
			fseek(fp, 0L, SEEK_END);
			sz = ftell(fp);
			progress = ((float) sz) / (float) fsize * 100;
			snprintf(out, 6, "%f\n", progress);
			writeblock(sock, out);
		}
		i++;
	}
	fclose(fp);

	printf("Finished recieved %d packets\n", i);
	writeblock(sock, "File transfer completed, attempting to print\n");
	bzero(sysout, 550);
	strcat(sysout, "lp -c -d Generic-2 ");
	strcat(sysout, fname);
	int status = system(sysout);
	//int status = 0;
	sleep(2);
	if (status < 0){
		writeblock(sock, "Printer error");
		error("ERROR printing failed");
	}
	writeblock(sock, "Print succeeded");
}

int main(int argc, char *argv[])
{
	int sockfd, newsockfd, portno, pid;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	struct sockaddr_in
	{
		short	sin_family;
		u_short	sin_port;
		struct 	in_addr sin_addr;
		char 	sin_zero[8];
	};


	if (argc < 2) {
		fprintf(stderr, "Error no port provided\n");
		exit(1);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		error("ERROR opening socket");

	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = atoi(argv[1]);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		error("ERROR on binding");

	listen(sockfd, 5);
	signal(SIGCHLD,SIG_IGN);
	clilen = sizeof(cli_addr);
	while (1) {
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if (newsockfd < 0)
			error("ERROR on accept");

		pid = fork();
		if (pid < 0)
			error("ERROR on fork");
		if (pid == 0)
		{
			close(sockfd);
			recieve_file(newsockfd);
			exit(0);
		}

	}

	return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

void error(char * msg)
{
	perror(msg);
	exit(0);
}

void readblock(int sock, char *buffer)
{
	int n;

	bzero(buffer, 512);
	n = read(sock, buffer, 512);
	if (n < 0)
		error("ERROR reading from socket");
	printf("%s\n", buffer);
}

void writeblock(int sock, char *omsg)
{
	int n;

	n = write(sock, omsg, 510);
	if (n < 0)
		error("ERROR writing to socket");
}

void transfer(int sock, char *filename)
{
	int err;
	char buffer[512];
	size_t sz;
	FILE *fp;
	FILE *ft;

	ft = fopen("temp.txt", "wb");

	printf("Sending filename...\n");
	writeblock(sock, filename);
	readblock(sock, buffer);
	if (strstr(buffer, "Ready to recieve filesize") == NULL)
		error("ERROR unexpected response");

	fp = fopen(filename, "rb");
	fseek(fp, 0L, SEEK_END);
	sz = ftell(fp);

	printf("Sending filesize...\n");
	bzero(buffer, 512);
	snprintf(buffer, 512, "%d", sz);
	writeblock(sock, buffer);
	readblock(sock, buffer);
	if (strstr(buffer, "Ready to recieve file") == NULL)
		error("ERROR unexpected response");

	rewind(fp);
	while (ftell(fp) < sz)
	{
		bzero(buffer, 512);
		if ((err = fread(buffer, 510, 1,fp)) < 0)
			error("ERROR read error");
		//printf("%s\n", buffer);
		//fwrite(buffer, sizeof(char), 510, ft);
		writeblock(sock, buffer);
		readblock(sock, buffer);
	}
	printf("Loop finished\n");
	readblock(sock, buffer);
	sleep(1);
	fclose(fp);
}

int main(int argc, char *argv[])
{
	int sockfd, portno, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	struct hosent
	{
		char	*h_name;
		char	**h_aliases;
		int	h_addrtype;
		int 	h_length;
		char	**h_addr_list;
		#define	h_addr 	h_addr_list[0]
	};

	char filename[512];

	if (argc < 4) {
		fprintf(stderr, "USAGE: %s hostname port filename\n", argv[0]);
		exit(0);
	}

	strcpy(filename, argv[3]);

	portno = atoi(argv[2]);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		error("ERROR opening socket");

	server = gethostbyname(argv[1]);
	if (server == NULL) {
		fprintf(stderr, "ERROR: no such host (%s)", argv[1]);
		exit(0);
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy(	(char * ) server->h_addr,
		(char *) &serv_addr.sin_addr.s_addr,
		server->h_length);
	serv_addr.sin_port = htons(portno);

	if (connect(sockfd, &serv_addr, sizeof(serv_addr)) < 0)
		error("ERROR connecting");

	printf("Connected! Starting Transfer\n");

	printf("Filename %s\n", filename);
	transfer(sockfd, filename);

	return 0;
}
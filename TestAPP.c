/*
** client.c -- a stream socket client demo
** Origin: http://beej.us/guide/bgnet/examples/client.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#include <signal.h>

#define PORT "8481" // the port client will be connecting to
#define HOST "192.168.8.21"

#define MAXDATASIZE 100 // max number of bytes we can get at once

#define GET_MY_DEVICE_CMD_BUF_SIZE		1024
char cmdFindMyDevice[GET_MY_DEVICE_CMD_BUF_SIZE];
#define CMD_FIND_MY_DEVICE	\
	"{"\
		"\"cmd\": \"find_my_device\""\
	"}"

static unsigned terminate = 0;


void sigroutine(int signo) {
	switch (signo) {
	case SIGHUP:
		printf("Get a signal -- SIGHUP ");
		break;
	case SIGINT:
		printf("Get a signal -- SIGINT ");
		break;
	case SIGQUIT:
		printf("Get a signal -- SIGQUIT ");
		break;
	case SIGTERM:
		printf("Get a signal -- SIGTERM ");
		break;
	}

	terminate = 1;
	return;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int make_cmdFindMyDevice(int sid)
{
	int rv =
	snprintf(
		cmdFindMyDevice,
		GET_MY_DEVICE_CMD_BUF_SIZE,
		"DL: %ld\r\n"
		"PT: JSON\r\n"
		"PC: NA\r\n"
		"SID: %d\r\n\r\n"
		CMD_FIND_MY_DEVICE,
		strlen(CMD_FIND_MY_DEVICE),
		sid
	);
	// printf("cmdFindMyDevice= [%s]\n", cmdFindMyDevice);

	return rv;
}

int main(int argc, char *argv[])
{
	int sockfd, numbytes;
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];
	int sid =0;

	// if (argc != 2) {
	//     fprintf(stderr,"usage: client hostname\n");
	//     exit(1);
	// }

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(HOST, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			perror("client: connect");
			close(sockfd);
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure

	while (1) {
		if (terminate)
			break;

		rv = make_cmdFindMyDevice(++sid);
		printf("rv= %d\n, cmd= [%s] \n", rv, cmdFindMyDevice);

		rv = send(sockfd, cmdFindMyDevice, rv, MSG_NOSIGNAL);

		if (rv == -1) {
			printf("send cmdFindMyDevice failed!\n");
			break;
		}

		sleep(10);
	}

	// if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
	//     perror("recv");
	//     exit(1);
	// }

	// buf[numbytes] = '\0';

	// printf("client: received '%s'\n",buf);

	close(sockfd);

	return 0;
}
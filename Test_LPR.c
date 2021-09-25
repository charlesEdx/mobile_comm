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
#define HOST "192.168.80.131"


#define JSON_CMD_STRING_BUF_SIZE		1024
char jsonCmdString[JSON_CMD_STRING_BUF_SIZE];


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

int make_jsonCmd_GetAI(int sid)
{
	#define CMD_GET_AI	\
	"{"\
		"\"cmd\": \"get_ai\""\
	"}"

	int rv =
	snprintf(
		jsonCmdString,
		JSON_CMD_STRING_BUF_SIZE,
		"DL: %ld\r\n"
		"PT: JSON\r\n"
		"PC: NA\r\n"
		"SID: %d\r\n\r\n"
		CMD_GET_AI,
		strlen(CMD_GET_AI),
		sid
	);
	//printf("jsonCmd_GetAI= [%s]\n", jsonCmdString);

	return rv;
}


#define MAXDATASIZE 2048 // max number of bytes we can get at once
static char rcvBuf[MAXDATASIZE];

int test_GetAI(int sockfd, int sid)
{
	int rv;
	int n_rcv;

	rv = make_jsonCmd_GetAI(sid);
	printf("\n>>>> Sending cmd= \n[%s] \n", jsonCmdString);

	rv = send(sockfd, jsonCmdString, rv, MSG_NOSIGNAL);
	if (rv == -1) {
		printf("ERROR: send jsonCmdString failed!\n");
		return -1;
	}

	if ((n_rcv = recv(sockfd, rcvBuf, MAXDATASIZE-1, 0)) == -1) {
	    perror("recv");
	    return -1;
	}

	rcvBuf[n_rcv] = '\0';

	printf("\n>>>> Received:\n'%s'\n",rcvBuf);

}

int make_jsonCmd_UpdateAI(int sid)
{
	#define CMD_UPDATE_AI	\
	"{"\
		"\"cmd\": \"update_ai\","\
		"\"function\": \"LPR\","\
		"\"license\": \"AAA-1111\","\
		"\"name\": \"Jerry\""\
	"}"

	int rv =
	snprintf(
		jsonCmdString,
		JSON_CMD_STRING_BUF_SIZE,
		"DL: %ld\r\n"
		"PT: JSON\r\n"
		"PC: NA\r\n"
		"SID: %d\r\n\r\n"
		CMD_UPDATE_AI,
		strlen(CMD_UPDATE_AI),
		sid
	);
	//printf("jsonCmd_GetAI= [%s]\n", jsonCmdString);

	return rv;
}


int test_UpdateAI(int sockfd, int sid)
{
	int rv;
	int n_rcv;

	rv = make_jsonCmd_UpdateAI(sid);
	printf("\n>>>> Sending cmd= \n[%s] \n", jsonCmdString);

	rv = send(sockfd, jsonCmdString, rv, MSG_NOSIGNAL);
	if (rv == -1) {
		printf("ERROR: send jsonCmdString failed!\n");
		return -1;
	}

	if ((n_rcv = recv(sockfd, rcvBuf, MAXDATASIZE-1, 0)) == -1) {
	    perror("recv");
	    return -1;
	}

	rcvBuf[n_rcv] = '\0';

	printf("\n>>>> Received:\n'%s'\n",rcvBuf);

}



int make_jsonCmd_SetAI(int sid)
{
	#define CMD_SET_AI	\
	"{"\
		"\"cmd\": \"set_ai\","\
		"\"function\": \"LPR\","\
		"\"license\": \"AAA-1111\","\
		"\"name\": \"Herry\""\
	"}"

	int rv =
	snprintf(
		jsonCmdString,
		JSON_CMD_STRING_BUF_SIZE,
		"DL: %ld\r\n"
		"PT: JSON\r\n"
		"PC: NA\r\n"
		"SID: %d\r\n\r\n"
		CMD_SET_AI,
		strlen(CMD_SET_AI),
		sid
	);
	//printf("jsonCmd_GetAI= [%s]\n", jsonCmdString);

	return rv;
}


int test_SetAI(int sockfd, int sid)
{
	int rv;
	int n_rcv;

	rv = make_jsonCmd_SetAI(sid);
	printf("\n>>>> Sending cmd= \n[%s] \n", jsonCmdString);

	rv = send(sockfd, jsonCmdString, rv, MSG_NOSIGNAL);
	if (rv == -1) {
		printf("ERROR: send jsonCmdString failed!\n");
		return -1;
	}

	if ((n_rcv = recv(sockfd, rcvBuf, MAXDATASIZE-1, 0)) == -1) {
	    perror("recv");
	    return -1;
	}

	rcvBuf[n_rcv] = '\0';

	printf("\n>>>> Received:\n'%s'\n",rcvBuf);

}





int make_jsonCmd_DeleteAI(int sid)
{
	#define CMD_DELETE_AI	\
	"{"\
		"\"cmd\": \"delete_ai\","\
		"\"function\": \"LPR\","\
		"\"license\": \"AAA-1111\""\
	"}"

	int rv =
	snprintf(
		jsonCmdString,
		JSON_CMD_STRING_BUF_SIZE,
		"DL: %ld\r\n"
		"PT: JSON\r\n"
		"PC: NA\r\n"
		"SID: %d\r\n\r\n"
		CMD_DELETE_AI,
		strlen(CMD_DELETE_AI),
		sid
	);
	//printf("jsonCmd_GetAI= [%s]\n", jsonCmdString);

	return rv;
}


int test_DeleteAI(int sockfd, int sid)
{
	int rv;
	int n_rcv;

	rv = make_jsonCmd_DeleteAI(sid);
	printf("\n>>>> Sending cmd= \n[%s] \n", jsonCmdString);

	rv = send(sockfd, jsonCmdString, rv, MSG_NOSIGNAL);
	if (rv == -1) {
		printf("ERROR: send jsonCmdString failed!\n");
		return -1;
	}

	if ((n_rcv = recv(sockfd, rcvBuf, MAXDATASIZE-1, 0)) == -1) {
	    perror("recv");
	    return -1;
	}

	rcvBuf[n_rcv] = '\0';

	printf("\n>>>> Received:\n'%s'\n",rcvBuf);

}



int main(int argc, char *argv[])
{
	int sockfd, numbytes;
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

	int m = 0;
	while (1) {
		if (terminate)
			break;

		printf("\n==== m= %d =============\n", m);
		test_GetAI(sockfd, sid++);

		if (m % 3 == 0) {
			test_UpdateAI(sockfd, sid++);
		}
		else if (m % 3 == 1) {
			test_SetAI(sockfd, sid++);
		}
		else if (m % 3 == 2) {
			test_DeleteAI(sockfd, sid++);

			test_GetAI(sockfd, sid++);
		}

		m++;
		sleep(10);
	}


	close(sockfd);

	return 0;
}
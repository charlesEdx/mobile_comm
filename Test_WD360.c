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
//#define HOST "192.168.50.95"


#define JSON_CMD_STRING_BUF_SIZE		1024
char jsonCmdString[JSON_CMD_STRING_BUF_SIZE];


static unsigned terminate = 0;
#define MAXDATASIZE (5*1024*1024) // max number of bytes we can get at once
static char rcvBuf[MAXDATASIZE];

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


//---------------------------------------------
//-- Get Dewarp mode
//---------------------------------------------
int make_jsonCmd_GetDewarp(int sid)
{
	#define CMD_GET_DEWARP	\
	"{"\
		"\"cmd\": \"get_dewarp\""\
	"}"

	int rv =
	snprintf(
		jsonCmdString,
		JSON_CMD_STRING_BUF_SIZE,
		"DL: %ld\r\n"
		"PT: JSON\r\n"
		"PC: NA\r\n"
		"SID: %d\r\n\r\n"
		CMD_GET_DEWARP,
		strlen(CMD_GET_DEWARP),
		sid
	);
	//printf("jsonCmd_GetDewarp= [%s]\n", jsonCmdString);

	return rv;
}


int test_GetDewarp(int sockfd, int sid)
{
	int rv;
	int n_rcv;

	rv = make_jsonCmd_GetDewarp(sid);
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


//---------------------------------------------
//-- Set Dewarp Mode
//---------------------------------------------
int make_jsonCmd_SetDewarp(int sid, int dwp)
{
	#define CMD_SET_DEWARP	\
	"{"\
		"\"cmd\": \"set_dewarp\","\
		"\"dewarp1-1\": \"%d\""\
	"}"

	char cc[1024];
	snprintf(cc, 1024, CMD_SET_DEWARP, dwp);

	int rv =
	snprintf(
		jsonCmdString,
		JSON_CMD_STRING_BUF_SIZE,
		"DL: %ld\r\n"
		"PT: JSON\r\n"
		"PC: NA\r\n"
		"SID: %d\r\n\r\n"
		"%s",
		strlen(cc),
		sid, cc
	);
	//printf("jsonCmd_SetDewarp= [%s]\n", jsonCmdString);

	return rv;
}


int test_SetDewarp(int sockfd, int sid, int dwp)
{
	int rv;
	int n_rcv;

	rv = make_jsonCmd_SetDewarp(sid, dwp);
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




//---------------------------------------------
//-- Get Air Value
//---------------------------------------------
int make_jsonCmd_GetAirValue(int sid)
{
	#define CMD_GET_AIR_VALUE	\
	"{"\
		"\"cmd\": \"get_air_value\""\
	"}"

	int rv =
	snprintf(
		jsonCmdString,
		JSON_CMD_STRING_BUF_SIZE,
		"DL: %ld\r\n"
		"PT: JSON\r\n"
		"PC: NA\r\n"
		"SID: %d\r\n\r\n"
		CMD_GET_AIR_VALUE,
		strlen(CMD_GET_AIR_VALUE),
		sid
	);
	//printf("jsonCmd_GetAirValue= [%s]\n", jsonCmdString);

	return rv;
}


int test_GetAirValue(int sockfd, int sid)
{
	int rv;
	int n_rcv;

	rv = make_jsonCmd_GetAirValue(sid);
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

	printf("\n>>>> Received:\n%s\n",rcvBuf);

}


//---------------------------------------------
//-- Set Panel Display
//---------------------------------------------
int make_jsonCmd_SetDisplay(int sid, int disp)
{
	#define CMD_SET_DISPLAY	\
	"{"\
		"\"cmd\": \"set_panel\","\
		"\"display\": \"%d\""\
	"}"

	char cc[512];
	snprintf(cc, 512, CMD_SET_DISPLAY, disp);

	int rv =
	snprintf(
		jsonCmdString,
		JSON_CMD_STRING_BUF_SIZE,
		"DL: %ld\r\n"
		"PT: JSON\r\n"
		"PC: NA\r\n"
		"SID: %d\r\n\r\n"
		"%s",
		strlen(cc),
		sid, cc
	);
	//printf("jsonCmd_SetDisplay= [%s]\n", jsonCmdString);

	return rv;
}


int test_SetDisplay(int sockfd, int sid, int disp)
{
	int rv;
	int n_rcv;

	rv = make_jsonCmd_SetDisplay(sid, disp);
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




//---------------------------------------------
//-- Get Panel Display
//---------------------------------------------
int make_jsonCmd_GetDisplay(int sid)
{
	#define CMD_GET_DISPLAY	\
	"{"\
		"\"cmd\": \"get_panel_display\""\
	"}"

	int rv =
	snprintf(
		jsonCmdString,
		JSON_CMD_STRING_BUF_SIZE,
		"DL: %ld\r\n"
		"PT: JSON\r\n"
		"PC: NA\r\n"
		"SID: %d\r\n\r\n"
		CMD_GET_DISPLAY,
		strlen(CMD_GET_DISPLAY),
		sid
	);
	//printf("jsonCmd_GetDISPLAY= [%s]\n", jsonCmdString);

	return rv;
}

int test_GetDisplay(int sockfd, int sid)
{
	int rv;
	int n_rcv;

	rv = make_jsonCmd_GetDisplay(sid);
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

	printf("\n>>>> %d Received:\n'%.300s'\n",n_rcv, rcvBuf);
}

void flush_recv_soocket(int sockfd)
{
	struct timeval tv;
	fd_set fds;
	int ret;

	tv.tv_sec = 1;
	tv.tv_usec = 0;

	while(1) {
		FD_ZERO(&fds);
		FD_SET(sockfd, &fds);

		ret = select(sockfd + 1, &fds, NULL, NULL, &tv);
		if (-1 == ret) {
			printf("select errors!!\n");
			break;
		}
		else if (0 == ret) {
			// time-out
			break;
		}

		if (FD_ISSET(sockfd, &fds)) {
			ret = recv(sockfd, rcvBuf, MAXDATASIZE-1, 0);
			if (ret == -1) {
				perror("recv");
				break;
			}
			printf("***** flush size= %d\n", ret);
		}
	}

	return;
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

	flush_recv_soocket(sockfd);

	int m = 0, dv=4;
	while (1) {
		if (terminate)
			break;

		printf("\n==== m= %d =============\n", m);
		test_GetAirValue(sockfd, sid++);

		if (m % dv == 0) {
			test_GetDewarp(sockfd, sid++);
		}
		else if (m % dv == 1) {
			test_SetDewarp(sockfd, sid++, 11);
		}
		else if (m % dv == 2) {
			test_GetDisplay(sockfd, sid++);
		}
		else if (m % dv == 3) {
			test_SetDisplay(sockfd, sid++, 5);
		}

		m++;
		sleep(5);
	}


	close(sockfd);

	return 0;
}
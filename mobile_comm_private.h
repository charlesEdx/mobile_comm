#ifndef _MOBILE_COMM_PRIVATE_H_
#define _MOBILE_COMM__MOBILE_COMM_PRIVATE_H_H_

#include <sys/socket.h>
#include <netinet/in.h>

#ifdef __cplusplus
extern "C"
{
#endif



typedef struct conx_cb_ {
	char recvBuf[LPR_JSON_DATA_SIZE];		//char *recvBuf;
	char jsonDataBuf[LPR_JSON_DATA_SIZE];	// char *jsonDataBuf;
	fd_set fdsr;
	// struct timeval timeout;
	struct sockaddr_in clientInfo;
	int addrlen;
//		int ret;
	int clientSockfd;
	int jsonDataLen;
	int findHeader;
	int recvTimeoutCount;
	int cmdDataLen;
} conx_cb_t;


#define recvBuf				(conx_cb.recvBuf)
#define jsonDataBuf			(conx_cb.jsonDataBuf)
#define fdsr				(conx_cb.fdsr)
// #define timeout				(conx_cb.timeout)
#define clientInfo			(conx_cb.clientInfo)
#define addrlen				(conx_cb.addrlen)
#define clientSockfd		(conx_cb.clientSockfd)
#define jsonDataLen			(conx_cb.jsonDataLen)
#define findHeader			(conx_cb.findHeader)
#define recvTimeoutCount	(conx_cb.recvTimeoutCount)
#define cmdDataLen			(conx_cb.cmdDataLen)





#ifdef __cplusplus
}
#endif

#endif  // _MOBILE_COMM_PRIVATE_H_

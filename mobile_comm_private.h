#ifndef _MOBILE_COMM_PRIVATE_H_
#define _MOBILE_COMM_PRIVATE_H_

#include <sys/socket.h>
#include <netinet/in.h>

#include "mobile_comm.h"

#ifdef __cplusplus
extern "C"
{
#endif


//---------------------------------------------
//-- define a struct to collect and pass global data amoung routines
//---------------------------------------------
typedef struct conx_cb_ {
	char _recvBuf[CSVR_RECV_CMDBUF_SIZE];			// the buffer to store received command packet from mobile app
	char _recvJsonBuf[CSVR_RECV_JSON_CMD_SIZE];		// the buffer to store whole json cmd from mobile app
	int  _recvJsonLen;			// total data received

	fd_set fdsr;									// readset of select()
	struct sockaddr_in clientInfo;
	int addrlen;

	int clientSockfd;
	int findHeader;
	int recvTimeoutCount;
	int _cmdDataLen;
} conx_cb_t;

//---------------------------------------------
//-- alias to global data
//---------------------------------------------
extern conx_cb_t conx_cb;

#define g_recvBuf			(conx_cb._recvBuf)
#define g_recvJsonBuf		(conx_cb._recvJsonBuf)
#define g_recvJsonLen		(conx_cb._recvJsonLen)

#define fdsr				(conx_cb.fdsr)
// #define timeout				(conx_cb.timeout)
#define clientInfo			(conx_cb.clientInfo)
#define addrlen				(conx_cb.addrlen)
#define clientSockfd		(conx_cb.clientSockfd)
#define g_hdrReceived		(conx_cb.findHeader)
#define recvTimeoutCount	(conx_cb.recvTimeoutCount)
#define g_cmdDL			(conx_cb._cmdDataLen)





#ifdef __cplusplus
}
#endif

#endif  // _MOBILE_COMM_PRIVATE_H_

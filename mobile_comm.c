#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>

#include <unistd.h>	// getopt, usleep

#include <jansson.h>
#include <stdbool.h>

#include <string.h>
#include <strings.h>
#include <sys/sysinfo.h>

#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <errno.h>

#include "mobile_comm.h"
#include "mobile_comm_private.h"


//---------------------------------------------
//-- In-File Debug Printf
//---------------------------------------------
#define LOG_INFO	1
#define LOG_DBG		1
#define LOG_VERBOSE	1
#define LOG_FUNC	0
#include "infile_debug.h"

conx_cb_t conx_cb;


//---------------------------------------------
//-- Sockets
//---------------------------------------------
static int g_controlSVR_FD = -1;		// the socket fd to listen connection request
static int g_clientSocket_FD = -1;		// the socket fd of the accepted connection to send() and recv() data.
static int g_controlSVR_Running = 0;
static pthread_mutex_t g_clientSocket_mutex;

static int g_cmdSID;




///!------------------------------------------------------
///! @brief		CSVR_ReplyResult
///! @param
///! @return
///! @note
///!------------------------------------------------------
int CSVR_ReplyResult(char *respJsonBuf, unsigned int respJsonSize, char *bindata, long int binsize, int sid)
{
	int ret = 0;
	char *returnMsg;
	long int msgLen;

#ifdef PRINT_FDFR_SOCKET_SEND_TIME
	struct timeval t1, t2;
	double prodcessTime = 0;
#endif

#ifdef PRINT_FDFR_SOCKET_SEND_TIME
	gettimeofday(&t1, NULL);
#endif
	returnMsg = NULL;
	verbose_printf("[%s] respJsonSize=%d, binsize=%ld \n", __FUNCTION__, respJsonSize, binsize);

	if (respJsonBuf == NULL) {
		error_printf("respJsonBuf is NULL!\n");
		return -1;
	}

	if (pthread_mutex_lock(&g_clientSocket_mutex) != 0) {
		error_printf("pthread_mutex_lock() error!\n");
		return -1;
	}

	if (g_clientSocket_FD != -1) {
		int ret;
		int len;
		struct timeval tv;
		fd_set fds;
		long int returnMsgLen;

		/* Prepare the message of protocol */
		msgLen = CSVR_RESP_JSON_HEADER_MAX_SIZE+respJsonSize+(bindata ? binsize : 0);
		returnMsg = (char *)calloc(sizeof(char), msgLen);
		snprintf(returnMsg,
			msgLen,
			"DL: %ld\r\n"
			"PT: JSON\r\n"
			"PC: NA\r\n"
			"SID: %d\r\n\r\n"
			"%s",
			respJsonSize+binsize,
			sid,
			respJsonBuf);

		returnMsgLen = strlen(returnMsg);

		if (bindata) {
			memcpy(returnMsg+returnMsgLen, bindata, binsize);
			returnMsgLen += binsize;
		}

		debug_printf("returnMsg size=%ld, data=\n%s\n", returnMsgLen, returnMsg);

		tv.tv_sec = 1;
		tv.tv_usec = 0;

		FD_ZERO(&fds);
		FD_SET(g_clientSocket_FD, &fds);

		ret = select(g_clientSocket_FD + 1, NULL, &fds, NULL, &tv);

		if ((ret > 0) && FD_ISSET(g_clientSocket_FD, &fds)) {
			// Socket is writable
			verbose_printf("socket is writable");

			len = send(g_clientSocket_FD, returnMsg, returnMsgLen, MSG_NOSIGNAL);
			//verbose_printf("send() = %d\n", len);

			if (len == -1) {
				error_printf("send data failed!\n");
				ret = -1;
				goto exit_func;
			}
		}
		else {
			//Socket is not writable
			error_printf("socket is not writable!\n");
			ret = -1;
			goto exit_func;
		}
	}
	else {
		error_printf("no connection exist!\n");
		ret = -1;
		goto exit_func;
	}

	ret = 0;

exit_func:
	if (returnMsg) {
		free(returnMsg);
	}
	if (ret != 0) {
		//if (g_FDFRClientSockFD != -1) {
		//    close(g_FDFRClientSockFD);
		//    g_FDFRClientSockFD = -1;
		//}
	}

	pthread_mutex_unlock(&g_clientSocket_mutex);

#ifdef PRINT_FDFR_SOCKET_SEND_TIME
	gettimeofday(&t2, NULL);
	prodcessTime = ((t2.tv_sec - t1.tv_sec) * 1000 + (double)(t2.tv_usec - t1.tv_usec) / 1000);
	DBG("take time: %f ms\n", prodcessTime);
#endif

	return ret;
}




///!------------------------------------------------------
///! @brief		CSVR_ProcessRequest
///! @param
///! @return
///! @note
///!------------------------------------------------------
void CSVR_ProcessRequest(char *jsonData, int length, int sid)
{
	extern void LPR_ProcessRequest(char *jsonData, int length, int sid);
	LPR_ProcessRequest(jsonData, length, sid);
}


///!------------------------------------------------------
///! @brief		_csvr_accept_connection
///! @param
///! @return
///! @note
///!------------------------------------------------------
static void _accept_connection(void)
{
	//--- CY: why check FD_ISSET(g_controlSVR_Running)?? if (!FD_ISSET(g_controlSVR_Running, &fdsr)) {
	if (!g_controlSVR_Running) {
		error_printf("g_controlSVR_Running is not set !!\n");
		sleep(CSVR_SELECT_ERROR_RETRY_INTVAL);
		return;
	}

	addrlen = sizeof(clientInfo);	// CY: TODO, 這個 clientInfo 命名不太明確，其實就是 client 的 socket address strcuture

	//-- accept socket connection from APP
	verbose_printf("accepting client...\n");
	clientSockfd = accept(g_controlSVR_FD, (struct sockaddr*) &clientInfo, (socklen_t *)&addrlen);

	if (clientSockfd == -1) {
		error_printf("accept g_controlSVR_Running failed !!\n");
		return;
	}

	verbose_printf("a new client is connected -- 0x%0X\n", clientSockfd);

	if (pthread_mutex_lock(&g_clientSocket_mutex) == 0) {
		if (g_clientSocket_FD == -1) {
			g_clientSocket_FD = clientSockfd;
			verbose_printf("Setting g_clientSocket_FD .. 0x%0X\n", g_clientSocket_FD);
		}
		else {
			debug_printf("[WARNING] there was a client connected already, disconect old client!\n");
			close(g_clientSocket_FD);
			g_clientSocket_FD = clientSockfd;
		}

		// reset command parsing
		g_recvJsonLen = 0;
		g_hdrReceived = 0;
		recvTimeoutCount = 0;

		pthread_mutex_unlock(&g_clientSocket_mutex);
	}
	else {
		error_printf("pthread_mutex_lock() error!\n");
		close(clientSockfd);
	}

	return;
}


///!------------------------------------------------------
///! @brief		_process_starting_packet
///! @param
///! @return
///! @note
///!------------------------------------------------------
static void _process_starting_packet(int recv_len)
{
	#define KV_SZ_LENGTH		16
	char szKeyDL[KV_SZ_LENGTH], szValDL[KV_SZ_LENGTH];
	char szKeyPT[KV_SZ_LENGTH], szValPT[KV_SZ_LENGTH];
	char szKeyPC[KV_SZ_LENGTH], szValPC[KV_SZ_LENGTH];
	char szKeySID[KV_SZ_LENGTH], szValSID[KV_SZ_LENGTH];
	char val[KV_SZ_LENGTH];

	g_hdrReceived = 1;
	g_recvJsonLen = 0;
	recvTimeoutCount = 0;

	memset(szKeyDL, 0, KV_SZ_LENGTH);
	memset(szValDL, 0, KV_SZ_LENGTH);
	memset(szKeyPT, 0, KV_SZ_LENGTH);
	memset(szValPT, 0, KV_SZ_LENGTH);
	memset(szKeyPC, 0, KV_SZ_LENGTH);
	memset(szValPC, 0, KV_SZ_LENGTH);
	memset(szKeySID, 0, KV_SZ_LENGTH);
	memset(szValSID, 0, KV_SZ_LENGTH);
	memset(g_recvJsonBuf, 0, CSVR_RECV_JSON_CMD_SIZE);

	/////////////////////////////////////////////////////////////
	/// Message example:
	/// 	DL: 200\r\n
	///		PT: JSON\r\n
	///		PC: NA\r\n
	///		SID: 99\r\n\r\n
	///		[Data...]
	int ret = sscanf(g_recvBuf, "%[^:]:%[^\r\n]\r\n%[^:]:%[^\r\n]\r\n%[^:]:%[^\r\n]\r\n%[^:]:%[^\r\n]\r\n",
					szKeyDL, szValDL, szKeyPT, szValPT, szKeyPC, szValPC, szKeySID, szValSID);

	#if 1 ///-- DEBUG ONLY
		printf("\n=== Received data ... \n%s\n", g_recvBuf);
		printf("%s = %s, %s = %s, %s = %s, %s = %s \n", szKeyDL, szValDL, szKeyPT, szValPT, szKeyPC, szValPC, szKeySID, szValSID);
	#endif

	if ( !(	(ret == 8)
			&& STR_EQUAL("DL", szKeyDL) && STR_EQUAL("PT", szKeyPT)
			&& STR_EQUAL("PC", szKeyPC) && STR_EQUAL("SID", szKeySID) ) ) {
		error_printf("cmd parse error! ret = %d\n", ret);
		return;
	}

	char *headerEnd = NULL;
	int dlVal = atoi(szValDL);
	int sidVal = atoi(szValSID);

	g_cmdSID = sidVal;

	verbose_printf("DL=%d, PT=%s, PC=%s, SID=%d ... \n", dlVal, szValPT, szValPC, sidVal);

	g_cmdDL = dlVal;

	headerEnd = strstr(g_recvBuf, "\r\n\r\n");
	if (NULL==headerEnd) {
		error_printf("can not find header end symbol (\\r\\n\\r\\n) ...\n");
		return;
	}

	verbose_printf( "found header end symbol (\\r\\n\\r\\n) ...\n");

	headerEnd += 4;		// pointed to start of cmd data
	int dataSize = strlen(headerEnd);
	if (dataSize <= 0) {
		// no json data ?
		error_printf("[FIXME] %s, %d !!\n", __FUNCTION__, __LINE__);
		return;
	}

	verbose_printf("copy json data len=%d \n", dataSize);
	memcpy( (g_recvJsonBuf+g_recvJsonLen), headerEnd, dataSize);
	g_recvJsonLen += dataSize;		// offset to the next position of the buffer to store data from next packet

	// recv json data finish
	if (g_cmdDL == g_recvJsonLen) {

		#ifdef MC_SAVE_JSON_CMD_TO_FILE
		FILE *pfile = NULL;

		pfile = fopen(MC_CMD_SAVE_JSON_PATH, "w");
		if (pfile != NULL)	{
			if(g_recvJsonLen > 0) {
				int writeLen = 0;

				writeLen += fwrite(g_recvJsonBuf, 1, g_recvJsonLen, pfile);
				// debug_printf("write len=%d\n", writeLen);
			}
			fclose(pfile);
		}
		#endif	// MC_SAVE_JSON_CMD_TO_FILE

		verbose_printf("recv json data finish, len=%d\n", g_recvJsonLen);
		verbose_printf("++++++++++ CSVR_ProcessRequest() \n");
		CSVR_ProcessRequest(g_recvJsonBuf, g_recvJsonLen, g_cmdSID);

		//reset command parsing
		g_recvJsonLen = 0;
		g_hdrReceived = 0;
		g_cmdSID = -1;
	}
	// else if (g_cmdDL > g_recvJsonLen) {
	// 	verbose_printf("[LPR_SRV] need to recv the reset of json datn\n");
	// }
	else if (g_cmdDL < g_recvJsonLen) {
		error_printf("json data length is more than DL length!, g_recvJsonLen=%d\n", g_recvJsonLen);
		//reset command parding
		g_recvJsonLen = 0;
		g_hdrReceived = 0;
		g_cmdSID = -1;
	}

	return;
}

///!------------------------------------------------------
///! @brief		_process_trailing_packet
///! @param
///! @return
///! @note
///!------------------------------------------------------
static void _process_trailing_packet(int recv_len)
{
	if (!g_hdrReceived) {
		error_printf("recv json data without header found! len=%d\n", recv_len);
		return;
	}

	if ( (g_recvJsonLen + recv_len) >= CSVR_RECV_JSON_CMD_SIZE ) {
		error_printf("json data buffer overflow! g_recvJsonLen+recv_len=%d\n", g_recvJsonLen + recv_len);

		// reset command parsing
		g_recvJsonLen = 0;
		g_hdrReceived = 0;
		recvTimeoutCount = 0;

		return;
	}

	verbose_printf("copy json data len=%d \n", recv_len);
	memcpy( (g_recvJsonBuf+g_recvJsonLen), g_recvBuf, recv_len);
	g_recvJsonLen += recv_len;

	//recv jsondat finish
	if (g_cmdDL != g_recvJsonLen) {
		error_printf("g_cmdDL(%d) != jsonDataLen(%d) !!\n", g_cmdDL, g_recvJsonLen);
		return;
	}

	#ifdef MC_SAVE_JSON_CMD_TO_FILE
	FILE *pfile = NULL;
	pfile = fopen(MC_CMD_SAVE_JSON_PATH, "w");
	if(pfile != NULL)
	{
		if(g_recvJsonLen > 0)
		{
			int writeLen = 0;
			writeLen += fwrite(g_recvJsonBuf, 1, g_recvJsonLen, pfile);
			DBG("[LPR_SRV] write len=%d\n", writeLen);

		}
		fclose(pfile);
	}
	#endif // MC_SAVE_JSON_CMD_TO_FILE

	verbose_printf("recv json data finish, len=%d\n", g_recvJsonLen);

	CSVR_ProcessRequest(g_recvJsonBuf, g_recvJsonLen, g_cmdSID);

	//reset command parsing
	g_recvJsonLen = 0;
	g_hdrReceived = 0;
	recvTimeoutCount = 0;

	return;
}




///!------------------------------------------------------
///! @brief		_process_connection
///! @param
///! @return
///! @note
///!------------------------------------------------------
static void _process_connection(void)
{
	//---------------------------------------------
	/// process incoming connection
	/// APP protocol: https://docs.google.com/document/d/1h8qM2tJFuIYJX0DSADmVdcMAKvoZXL3M/edit
	//---------------------------------------------
	if (!FD_ISSET(g_clientSocket_FD, &fdsr)) {
		//-- supposedly, it should not happen.
		error_printf(" client recv but g_clientSocket_FD not SET????\n");
		return;
	}

	//---------------------------------------------
	///-- receive message into buffer
	//---------------------------------------------
	memset(g_recvBuf, 0, CSVR_RECV_CMDBUF_SIZE);
	int recv_len = recv(g_clientSocket_FD, (void*) g_recvBuf, CSVR_RECV_CMDBUF_SIZE, 0);    //recv data from app
	verbose_printf("received %d data from client - 0x%0X\n", recv_len, g_clientSocket_FD);
	#if 0	///-- DEBUG ONLY
		if (recv_len) {
			printf("=== Received data ...\n %s\n\n", g_recvBuf);
			printf("=== Hex ... \n");
			int k;
			for (k=0; k<recv_len; k++) {
				printf("0x%02X ", g_recvBuf[k]);
			}
			puts("\n");
		}
	#endif

	//---------------------------------------------
	/// recv() failed
	//---------------------------------------------
	if (recv_len < 0) {
		if(errno == EINTR) {
			error_printf("Interrupted system call-- continue !!\n");
			return;	/* Interrupted system call */
		}

		error_printf("receive client data failed!!\n");
		close(g_clientSocket_FD);
		g_clientSocket_FD = -1;
		return;
	}

	//---------------------------------------------
	/// recv() : No data, session completed, client has disconnected
	//---------------------------------------------
	if (recv_len == 0) {
		// recv complete
		info_printf("(recv_len == 0), client disconnected\n");
		close(g_clientSocket_FD);
		g_clientSocket_FD = -1;

		// reset command parsing
		g_hdrReceived = 0;
		g_recvJsonLen = 0;
		return;
	}

	//---------------------------------------------
	/// Start to process received message
	//---------------------------------------------
	if ( (!g_hdrReceived) && (recv_len > 3) && (g_recvBuf[0] == 'D') && (g_recvBuf[1] == 'L') && (g_recvBuf[2] == ':') ) {
		///--------------------------------------------
		/// (1) !g_hdrReceived --> the starting packet of message.
		///--------------------------------------------
		verbose_printf("processing starting packet ...");
		_process_starting_packet(recv_len);
	}
	else
	{
		///--------------------------------------------
		/// (2) g_hdrReceived --> rest packets of seession (except the starting one)
		///--------------------------------------------
		verbose_printf("processing trailing packets ...");
		_process_trailing_packet(recv_len);
	}
}


///!------------------------------------------------------
///! @brief		_THREAD_INIT_ControlSVR
///! @param
///! @return
///! @note
///!------------------------------------------------------
static int _THREAD_INIT_ControlSVR(void *param)
{
	clientSockfd = -1;
	g_recvJsonLen = 0;
	g_hdrReceived = 0;
	g_cmdDL = 0;


	return 0;
}




///!------------------------------------------------------
///! @brief		The thread process of controlSVR
///! @param
///! @return
///! @note
///!------------------------------------------------------
static void* _THREAD_ControlSVR(void *param)
{
	int ret;

	//-- FUNC_ENTRY();

	////////////////////////////////////////////////
	///!--- Thread global initialization
	////////////////////////////////////////////////
	if (_THREAD_INIT_ControlSVR(param) < 0) {
		goto THREAD_EXIT;
	}

	////////////////////////////////////////////////
	///!--- Thread Loop
	////////////////////////////////////////////////
	struct timeval timeout;

	while (g_controlSVR_Running) {

		if (g_controlSVR_FD == -1) {
			error_printf("Exiting thread loop ...(g_controlSVR_FD == -1) !!!\n");
			break;
		}

		//---------------------------------------------
		//-- The app command could be segmented into multiple packets,
		//---------------------------------------------
		if (g_clientSocket_FD > 0) {
			/// client connected -- skip select(g_controlSVR_FD), unnecessary 2sec timeout !!!
			goto _PROC_CONNECTION;
		}

		FD_ZERO(&fdsr);						// clear socket readset
		FD_SET(g_controlSVR_FD, &fdsr);		// fdsr --> socket readset

		timeout.tv_sec  = (CSVR_SELECT_TIMEOUT_INTVAL / 1000);
		timeout.tv_usec = (CSVR_SELECT_TIMEOUT_INTVAL % 1000) * 1000;

		//---------------------------------------------
		//-- Waiting connection request from APP
		//-- select(g_controlSVR_FD)
		//---------------------------------------------
		verbose_printf("checking controlSVR socket status --> select(g_controlSVR_FD) ...\n");
		ret = select(g_controlSVR_FD + 1, &fdsr /*readset*/, NULL/*writeset*/, NULL/*exceptset*/, &timeout);
		verbose_printf("\t... select(g_controlSVR_FD) return=%d ...\n", ret);


		////////////////////////////////////////////////////
		/// (1) Select(g_controlSVR_FD) Error !!
		////////////////////////////////////////////////////
		if (ret == -1) {
			//--- select server error !!
			error_printf("checking socket of controlSVR error - errono=%d, %s \n", errno, strerror(errno));
			sleep(CSVR_SELECT_ERROR_RETRY_INTVAL);
			continue;
		}

		////////////////////////////////////////////////////
		/// (2) Select(g_controlSVR_FD) Timeout !!
		////////////////////////////////////////////////////
		if (ret == 0) {

			#if 0 // DEBUG ONLY
			printf("select(g_controlSVR_FD) time out!! ");
			putc('+', stdout);
			#endif

			if (g_clientSocket_FD <= 0) {
				verbose_printf("checking controlSVR socket timeout and no client connected ...\n");
				#if CSVR_SELECT_TIMEOUT_DWELL_INTVAL > 0
				usleep(CSVR_SELECT_TIMEOUT_DWELL_INTVAL*1000);
				#endif
				continue;
			}

//---------------------------------------------
//-- FIXME: 這裡的寫法不太乾淨，當處沒有想清楚，要找時間修一下！！
//---------------------------------------------
_PROC_CONNECTION:
			//---------------------------------------------
			//-- Wait connection from client
			//---------------------------------------------
			timeout.tv_sec  = (CSVR_SELECT_TIMEOUT_INTVAL / 1000);
			timeout.tv_usec = (CSVR_SELECT_TIMEOUT_INTVAL % 1000) * 1000;

			FD_ZERO(&fdsr);
			FD_SET(g_clientSocket_FD, &fdsr);
			ret = select(g_clientSocket_FD + 1, &fdsr, NULL, NULL, &timeout);

			if (ret == -1) {
				//--- error
				error_printf("check client connection error! --> closing client socket (%d) \n", g_clientSocket_FD);
				if (g_clientSocket_FD > 0) {
					close(g_clientSocket_FD);
				}
				g_clientSocket_FD = -1;
			}
			else if (ret == 0) {
				//--- timout
				if (g_hdrReceived || (g_recvJsonLen > 0)) {
					debug_printf("recv data timeout -- g_hdrReceived=%d, g_recvJsonLen=%d\n", g_hdrReceived, g_recvJsonLen);

					if (recvTimeoutCount > CSVR_CONNECTION_ABORT_TIMEOUT_COUNT) {
						//-- reset command paseing
						g_recvJsonLen = 0;
						g_hdrReceived = 0;

						error_printf("recv data timeout (count=%d > %d), reset g_hdrReceived flag\n", CSVR_CONNECTION_ABORT_TIMEOUT_COUNT, recvTimeoutCount);
					}
					else {
						recvTimeoutCount++;
					}
				}
			}
			else
			{
				//---------------------------------------------
				//-- process incoming connection
				//-- APP protocol: https://docs.google.com/document/d/1h8qM2tJFuIYJX0DSADmVdcMAKvoZXL3M/edit
				info_printf("processing connection\n");
				//---------------------------------------------
				_process_connection();
			}
			continue;
		}

		////////////////////////////////////////////////////
		// (3) Select(g_controlSVR_FD) found pending request !!
		////////////////////////////////////////////////////
		info_printf("accepting new connection\n");
		_accept_connection();
	}	/// while()

	if(g_controlSVR_FD != -1) {
		close(g_controlSVR_FD);
		g_controlSVR_FD = -1;
		verbose_printf("[FDFR_SRV] socket server is closed\n");
	}

THREAD_EXIT:
	FUNC_EXIT();
	pthread_exit(NULL);

	return NULL;
}



///!------------------------------------------------------
///! @brief		ControlSVR_Start
///! @param
///! @return
///! @note
///!------------------------------------------------------
int ControlSVR_Start(void)
{
	struct sockaddr_in serverInfo;
	pthread_attr_t attr;
	pthread_t pt_action_thread;

	//--- socket() --  create socket for controlSVR
	if (g_controlSVR_FD != -1) {
		verbose_printf("[%s] socket server was created! \n", __FUNCTION__);
		return 0;
	}

	verbose_printf("Creating socket on g_controlSVR_FD \n");
	g_controlSVR_FD = socket(AF_INET, SOCK_STREAM, 0);

	if (g_controlSVR_FD == -1) {
		error_printf("create socket error!!!\n");
		return -1;
	}

	//--- Configure socket parameters
	in_port_t server_port = CONTROL_SVR_PORT;
	bzero(&serverInfo, sizeof(serverInfo));
	serverInfo.sin_family = PF_INET;
	serverInfo.sin_addr.s_addr = INADDR_ANY;
	serverInfo.sin_port = htons(server_port);

	//--- bind() - socket to a specific port
	verbose_printf("Binding socket g_controlSVR_FD on port %d ...\n", server_port);
	if (bind(g_controlSVR_FD, (struct sockaddr *)&serverInfo, sizeof(serverInfo)) == -1) 	{
		error_printf("bind sockfd failed!!!\n");
		return -1;
	}
	verbose_printf("bind sockfd succeeded - port=%d ...\n", server_port);

	//--- listen()
	verbose_printf("listening socket g_controlSVR_FD  ... timeout=5-sec\n");
	if (listen(g_controlSVR_FD, 5) == -1) {
		error_printf("listen sockfd error!!!\n");
		return -1;
	}
	verbose_printf("listen sockfd succeeded \n");

	//--- Now, controlSVR is running
	verbose_printf("Set flag, g_controlSVR_Running == 1 \n");
	g_controlSVR_Running = 1;

	//--- Creating controlSVR thread
	info_printf("creating controlSVR thread\n");
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	if (pthread_create(&pt_action_thread, &attr, _THREAD_ControlSVR, NULL) != 0)
	{
		error_printf("create controlSVR thread failed!!!\n");
		pthread_attr_destroy(&attr);
		g_controlSVR_Running = 0;

		close(g_controlSVR_FD);
		g_controlSVR_FD = -1;
		return -1;
	}

	pthread_attr_destroy(&attr);

	info_printf("controlSVR is created, g_controlSVR_FD = %d ...\n", g_controlSVR_FD);

	return 0;
}



///!------------------------------------------------------
///! @brief		ControlSVR_Stop
///! @param
///! @return
///! @note
///!------------------------------------------------------
void ControlSVR_Stop(void)
{
	info_printf("Stopping controlSVR ...");
	g_controlSVR_Running = 0;
	info_printf(" completed!\n");
}



///------------------------------------------------------------------
/// @brief
/// @param
/// @return
/// @note
///------------------------------------------------------------------
int main(int argc, char **argv)
{
	//---------------------------------------------
	//-- Initialize global variables
	//---------------------------------------------

	//---------------------------------------------
	//-- Create socket for controlSVR to receive commands from mobile apps
	//---------------------------------------------
	info_printf("Starting controlSVR ...\n");
	ControlSVR_Start();

	while(1) {
		sleep(10);	// usleep(1000);
	}

	return 0;
}

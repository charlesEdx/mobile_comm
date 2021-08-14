#include <stdio.h>
#include <unistd.h>
// #include <jpeglib.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>

#include <unistd.h>	// getopt

// #include "path_query.h"
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
#include "mc_common.h"


//---------------------------------------------
//-- In-File Debug Printf
//---------------------------------------------
#define LOG_DBG		1
#define LOG_VERBOSE	0
#define LOG_FUNC	0

#define NULL_FUNCTION				do {} while(0)
#define error_printf(format, ...)   fprintf(stderr, "%s() @ %s, %d, ERROR: "format, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);

#if LOG_DBG==1
#define debug_printf(format, ...)		fprintf(stderr, format, ##__VA_ARGS__)
#else
#define debug_printf(format, ...)		NULL_FUNCTION
#endif

#if LOG_VERBOSE==1
#define verbose_printf(format, ...)	fprintf(stderr, format, ##__VA_ARGS__)
#else
#define verbose_printf(format, ...)	NULL_FUNCTION
#endif

#if LOG_FUNC==1
#define FUNC_ENTRY(format, ...)     fprintf(stderr, "Entering --> %s("format")\n", __FUNCTION__, ##__VA_ARGS__)
#define FUNC_EXIT()                 fprintf(stderr, "Exiting <-- %s()", __FUNCTION__)
#else
#define FUNC_ENTRY(format, ...)     NULL_FUNCTION
#define FUNC_EXIT()                 NULL_FUNCTION
#endif



//---------------------------------------------
//-- Sockets
//---------------------------------------------
static int g_controlSVR_FD = -1;		// the socket fd to listen connection request
static int g_clientSocket_FD = -1;		// the socket fd of the accepted connection to send() and recv() data.
static int g_controlSVR_Running = 0;
static int g_LPRProcessRunning = 0;
static pthread_mutex_t g_LPRSocketMutex;



static    char _cam_user_name[48]={0};
static    char _cam_user_passwd[48]={0};
static    char _snapshot_path[256]={0};
static    char _snapshot_path_esc[256];
static    char _relay_jpg_path[64]={0};
static    char _rgb888_path[64]={0};
static    char _rgb565_path[64]={0};
static    char _bbox_bmp_path[64]={0};

#define LPR_RECV_BUF_SIZE (128 * 1024) // 128K
#define LPR_JSON_DATA_SIZE (900 * 1024) // 900k

#define LPR_MAX_REG_USER_NUM	100



#define JSON_RESP_HDR_MAX_LENGTH	(256)
#define JSON_RESP_DATA_LENGTH		(500*1024) 	// assume JPG or 2000 license plate data
static char _respJsonData[JSON_RESP_DATA_LENGTH] = {0};
static char *_respJsonHdrFormat = \
			"DL: %d\r\n"
			"PT: JSON\r\n"
			"PC: NA\r\n"
			"SID: %d\r\n\r\n";
static char *_respJsonDataFormat =
			"{"
			"\"cmd\":\"%s\","
			"\"function\":\"LPR\","
			"\"data\":[%s]"
			"}";

static path_query_obj _db_obj;
static int _jsonSID;

static int _dewarp_mode;
static int _aqd_panel;	// air quality panel dispaly


int current_dbg_level = 5;		// why do I need it here !?
char *app_name="mobile_comm";	// why do I need it here !?




///!------------------------------------------------------
///! @brief		LPR_ControlServerSendResult
///! @param
///! @return
///! @note
///!------------------------------------------------------
int LPR_ControlServerSendResult(char *jsonResult, unsigned int jsonSize, char *bindata, long int binsize, int sid)
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

	TRACE("[%s] jsonSize=%d, binsize=%ld \n", __FUNCTION__, jsonSize, binsize);

	if (jsonResult == NULL) {
		DBG("[LPR_SRV] jsonResult is NULL!\n");
		return -1;
	}

	if (pthread_mutex_lock(&g_LPRSocketMutex) != 0) {
		DBG("[LPR_SRV] pthread_mutex_lock() error!\n");
		return -1;
	}

	if (g_clientSocket_FD != -1) {
		int selectRet;
		int len;
		struct timeval tv;
		fd_set fds;
		long int returnMsgLen;

		/* Prepare the message of protocol */
		msgLen = 256+jsonSize+(bindata ? binsize : 0);
		returnMsg = (char *)calloc(sizeof(char), msgLen);
		snprintf(returnMsg,
			msgLen,
			"DL: %d\r\n"
			"PT: JSON\r\n"
			"PC: NA\r\n"
			"SID: %d\r\n\r\n"
			"%s",
			jsonSize+binsize,
			sid,
			jsonResult);

		returnMsgLen = strlen(returnMsg);

		if (bindata) {
			memcpy(returnMsg+returnMsgLen, bindata, binsize);
			returnMsgLen += binsize;
		}

		DBG("[LPR_SRV] returnMsg size=%ld, data=\n%s\n", returnMsgLen, returnMsg);

		tv.tv_sec = 1;
		tv.tv_usec = 0;

		FD_ZERO(&fds);
		FD_SET(g_clientSocket_FD, &fds);

		selectRet = select(g_clientSocket_FD + 1, NULL, &fds, NULL, &tv);

		if ((selectRet > 0) && FD_ISSET(g_clientSocket_FD, &fds)) {
			// Socket is writable
			TRACE("[LPR_SRV] socket is writable");

			len = send(g_clientSocket_FD, returnMsg, returnMsgLen, MSG_NOSIGNAL);
//			TRACE("[LPR_SRV] send() = %d", len);
			//DBG("[LPR_SRV] send() = %d\n=\n%s\n=", len, returnMsg);

			if (len == -1) {
				DBG("[LPR_SRV] send data failed!\n");
				ret = -1;
				goto exit_func;
			}
		}
		else {
			//Socket is not writable
			DBG("[LPR_SRV] socket is not writable!\n");
			ret = -1;
			goto exit_func;
		}
	}
	else {
		DBG("[LPR_SRV] no connection exist!\n");
		ret = -1;
		goto exit_func;
	}

	//DBG("[LPR_SRV] send FDFR result ok\n");
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

	pthread_mutex_unlock(&g_LPRSocketMutex);

#ifdef PRINT_FDFR_SOCKET_SEND_TIME
	gettimeofday(&t2, NULL);
	prodcessTime = ((t2.tv_sec - t1.tv_sec) * 1000 + (double)(t2.tv_usec - t1.tv_usec) / 1000);
	DBG("[LPR_SRV] take time: %f ms\n", prodcessTime);
#endif

	return ret;
}



///!------------------------------------------------------
///! @brief		LPR_Return_GetDewarp
///! @param
///! @return
///! @note
///!------------------------------------------------------
int LPR_Return_GetDewarp(int sid)
{
	sprintf(_respJsonData,
				"{"
				"\"cmd\": \"get_dewarp\","
				"\"result\": 0,"
				"\"dewarp1-1\": \"%d\"}",
				_dewarp_mode);
	TRACE("[LPR_SRV] return GetAI:\n%s\n", _respJsonData);

	return LPR_ControlServerSendResult(_respJsonData, strlen(_respJsonData), NULL, 0, sid);
}



///!------------------------------------------------------
///! @brief		LPR_Return_SetDewarp
///! @param
///! @return
///! @note
///!------------------------------------------------------
int LPR_Return_SetDewarp(int result, int sid)
{
	char *jsonFormat = "{"
					   "\"cmd\": \"set_dewarp\","
					   "\"result\": %d"
					   "}";


	memset(_respJsonData, 0, sizeof(_respJsonData));
	sprintf(_respJsonData, jsonFormat, result);

	TRACE("[LPR_SRV] return SetDewarp:\n%s\n", _respJsonData);

	return LPR_ControlServerSendResult(_respJsonData, strlen(_respJsonData), NULL, 0, sid);
}



///!------------------------------------------------------
///! @brief		LPR_Return_GetPanelDisplay
///! @param
///! @return
///! @note
///!------------------------------------------------------
int LPR_Return_GetPanelDisplay(int sid)
{
	sprintf(_respJsonData,
				"{"
				"\"cmd\": \"get_panel_display\","
				"\"result\": 0,"
				"\"display\": \"%d\"}",
				_aqd_panel);
	TRACE("[LPR_SRV] return GetPanelDisplay:\n%s\n", _respJsonData);

	return LPR_ControlServerSendResult(_respJsonData, strlen(_respJsonData), NULL, 0, sid);
}



///!------------------------------------------------------
///! @brief		LPR_Return_SetPanel
///! @param
///! @return
///! @note
///!------------------------------------------------------
int LPR_Return_SetPanel(int result, int sid)
{
	char *jsonFormat = "{"
					   "\"cmd\": \"set_panel\","
					   "\"result\": %d"
					   "}";


	memset(_respJsonData, 0, sizeof(_respJsonData));
	sprintf(_respJsonData, jsonFormat, result);

	TRACE("[LPR_SRV] return SetPanel:\n%s\n", _respJsonData);

	return LPR_ControlServerSendResult(_respJsonData, strlen(_respJsonData), NULL, 0, sid);
}



///!------------------------------------------------------
///! @brief		LPR_Return_GetAirValue
///! @param
///! @return
///! @note
///!------------------------------------------------------
int LPR_Return_GetAirValue(int sid)
{
	sprintf(_respJsonData,
				"{"
				"\"cmd\": \"get_air_value\","
				"\"result\": 0,"
				"\"pm10\": \"%d\","
				"\"pm2.5\": \"%d\","
				"\"pm1.0\": \"%d\","
				"\"pm0.3\": \"%d\","
				"\"hcho\": \"%d\","
				"\"co\": \"%d\","
				"\"co2\": \"%d\","
				"\"tvoc\": \"%d\","
				"\"temperature\": \"%2d.%1d\","
				"\"humidity\": \"%d\"}",
				_val_pm10, _val_pm2d5, _val_pm1d0, _val_pm0d3,
				_val_hcho, _val_co, _val_co2, _val_tvoc,
				_val_tempInt, _val_tempDec, _val_hum);
	TRACE("[LPR_SRV] return GetAirValue:\n%s\n", _respJsonData);

	return LPR_ControlServerSendResult(_respJsonData, strlen(_respJsonData), NULL, 0, sid);
}





///!------------------------------------------------------
///! @brief
///! @param
///! @return
///! @note
///!------------------------------------------------------
int getJsonString(char *buf, char *key, char **value, int *length)
{
	char keyName[128] = {0};
	char *keyNamePtr = NULL;
	char *keyBuf = NULL;
	unsigned keyBufSize = 0;
	int ret = 0;
	int i;
	int colon_pos = 0, first_doublequotes_pos = 0, second_doublequotes_pos = 0;

	*value = NULL;
	*length = 0;

	// find key
	sprintf(keyName, "\"%s\"", key);
	keyNamePtr = strstr(buf, keyName);
	if (keyNamePtr == NULL) {
		DBG("[LPR_SRV] no \"%s\" in jason\n", key);
		return -1;
	}

	i = 0;
	while (1) {
		if ((keyNamePtr[i] != ',') && (keyNamePtr[i] != '}')) {
			i++;
		}
		else {
			break;
		}

		//if (i >= sizeof(keyBuf)) {
		//    DBG("[LPR_SRV] error \"%s\" value in json\n", key);
		//    return -1;
		//}
	}

	keyBufSize = i + 1;
	keyBuf = malloc(keyBufSize);
	if (keyBuf == NULL) {
		DBG("[LPR_SRV] malloc error!\n");
		return -1;
	}

	memset(keyBuf, 0, keyBufSize);
	memcpy(keyBuf, keyNamePtr, keyBufSize - 1);

	//DBG("[LPR_SRV] find key: %s, size=%d\n", keyBuf, keyBufSize);

	ret = -1;
	for (i = 0; i < keyBufSize; i++) {
		if (colon_pos == 0) {
			if (keyBuf[i] == ':') {
				colon_pos = i;
			}
		}
		else if ((colon_pos > 0) && (first_doublequotes_pos == 0)) {
			if (keyBuf[i] == '"') {
				// first double quotes
				first_doublequotes_pos = i;
			}
		}
		else if ((colon_pos > 0) && (first_doublequotes_pos > 0) && (second_doublequotes_pos == 0)) {
			if (keyBuf[i] == '"') {
				// second double quotes
				second_doublequotes_pos = i;
				ret = 0;
				break;
			}
		}
	}

	if (ret == 0) {
		int valBufSize = second_doublequotes_pos - first_doublequotes_pos;
		if (valBufSize > 0) {
			*value = malloc(valBufSize);
		}

		if (*value == NULL) {
			DBG("[LPR_SRV] malloc error! valBufSize=%d\n", valBufSize);
			ret = -1;
			goto FUNC_RETURN;
		}

		memset(*value, 0, valBufSize);
		memcpy(*value, keyBuf + first_doublequotes_pos + 1, valBufSize - 1);
		*length = valBufSize - 1;

		//DBG( "[LPR_SRV] colon_pos=%d, first_dq_pos=%d, second_dq_pos=%d", colon_pos, first_doublequotes_pos, second_doublequotes_pos);
		if (*length < 32) {
			//DBG("[LPR_SRV] getJsonString() value=%s", *value);
		}
		//DBG("[LPR_SRV] getJsonString() length=%d", *length);
	}
	else {
		ERROR("[LPR_SRV] getJsonString() fail to get string value of %s\n!", keyName);
	}

FUNC_RETURN:

	if (keyBuf) {
		free(keyBuf);
	}

	return ret;
}


static conx_cb_t conx_cb;


///--------------------------------------------------------------------------------------
/// @brief    :
/// @param    :
/// @return   :
/// @details  :
///--------------------------------------------------------------------------------------
static int _validate_dewarp_mode(int mode)
{
	if (mode==0 || (mode>9 && mode<13) || (mode>19 && mode<22) || (mode>29 && mode<34)) {
		return mode;
	}
	else {
		return -1;
	}
}


///!------------------------------------------------------
///! @brief		LPR_ProcessRequest
///! @param
///! @return
///! @note
///!------------------------------------------------------
void LPR_ProcessRequest(char *jsonData, int length, int sid)
{
	char *cmdVal = NULL;
	int   cmdLen = 0;

	// fixed image size
	int imgWidth;
	int imgHeight;

	imgWidth = 480;
	imgHeight = 640;


	if (getJsonString(jsonData, "cmd", &cmdVal, &cmdLen) == 0) {
		TRACE("[LPR_SRV] cmd: %s\n", cmdVal);

		if (STREQL(cmdVal, "get_dewarp")) {
			TRACE("get_dewarp\n");
			LPR_Return_GetDewarp(sid);
		}
		else if (STREQL(cmdVal, "set_dewarp")) {
			TRACE("set_dewarp\n");
			char *dewarp_mode;
			int  sz_len;

			if ( 0 == getJsonString(jsonData, "dewarp1-1", &dewarp_mode, &sz_len) ) {
				int dwm = atoi(dewarp_mode);
				TRACE("dwm ---> %d", dwm);
				if (_validate_dewarp_mode(dwm) < 0) {
					// invalide dewarp mode
					LPR_Return_SetDewarp(1 /*format error*/, sid);
				}
				else {
					_dewarp_mode = dwm;
					TRACE("_dewarp_mode = %d", _dewarp_mode);
					LPR_Return_SetDewarp(0 /*success*/, sid);
				}
			}
			else {
				LPR_Return_SetDewarp(1 /*format error*/, sid);
			}
			if (dewarp_mode)	free(dewarp_mode);
		}
		else if (STREQL(cmdVal, "get_panel_display")) {
			TRACE("get_panel_display\n");
			LPR_Return_GetPanelDisplay(sid);
		}
		else if (STREQL(cmdVal, "set_panel")) {
			TRACE("set_panel\n");
			char *display;
			int  sz_len;

			if ( 0 == getJsonString(jsonData, "display", &display, &sz_len) ) {
				int aqd = atoi(display);
				if (aqd>0 && aqd<20) {
					_aqd_panel = aqd;
					TRACE(" _aqd_panel ---> %d", _aqd_panel);
					LPR_Return_SetPanel(0 /*success*/, sid);
					#ifdef USE_SERIAL_COMM
					_serial_comm_set_display(_aqd_panel);
					#endif // USE_SERIAL_COMM
				}
				else {
					// invalide dewarp mode
					LPR_Return_SetPanel(1 /*format error*/, sid);
				}
			}
			else {
				LPR_Return_SetPanel(1 /*format error*/, sid);
			}
			if (display)	free(display);
		}
		else if (STREQL(cmdVal, "get_air_value")) {
			TRACE("get_air_value\n");
			#ifdef USE_SERIAL_COMM
			_serial_comm_get_air_value();
			#endif // USE_SERIAL_COMM
			LPR_Return_GetAirValue(sid);
		}
		else {
			DBG("[LPR_SRV] unsupported cmd: %s", cmdVal);
		}
	}

	//--------------------------------------------
	// !!! Don't forget to free cmdVal
	//--------------------------------------------
	if (cmdVal) {
		free(cmdVal);
//		TRACE("[LPR_SRV] free cmdVal");
	}
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

@@@ Review 到這裡！！！

	//-- accept socket connection from APP
	verbose_printf("accepting client...\n");
	clientSockfd = accept(g_controlSVR_FD, (struct sockaddr*) &clientInfo, &addrlen);

	if (clientSockfd == -1) {
		error_printf("accept g_controlSVR_Running failed !!\n");
		return;
	}

	TRACE("[LPR_SRV] a new client is connected -- 0x%0X\n", clientSockfd);

	if (pthread_mutex_lock(&g_LPRSocketMutex) == 0) {
		if (g_clientSocket_FD == -1) {
			g_clientSocket_FD = clientSockfd;
			TRACE("[LPR_SRV] Setting g_clientSocket_FD .. 0x%0X\n", g_clientSocket_FD);
		}
		else {
			DBG("[WARNING][LPR_SRV] there was a client connected already, disconect old client!\n");
			close(g_clientSocket_FD);
			g_clientSocket_FD = clientSockfd;
		}

		// reset command parsing
		jsonDataLen = 0;
		findHeader = 0;
		recvTimeoutCount = 0;

		pthread_mutex_unlock(&g_LPRSocketMutex);
	}
	else {
		ERROR("[FDFR_SRV] pthread_mutex_lock() error!\n");
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
	//DBG("[LPR_SRV] cmd header found\n");

	findHeader = 1;
	jsonDataLen = 0;
	recvTimeoutCount = 0;

	memset(szKeyDL, 0, KV_SZ_LENGTH);
	memset(szValDL, 0, KV_SZ_LENGTH);
	memset(szKeyPT, 0, KV_SZ_LENGTH);
	memset(szValPT, 0, KV_SZ_LENGTH);
	memset(szKeyPC, 0, KV_SZ_LENGTH);
	memset(szValPC, 0, KV_SZ_LENGTH);
	memset(szKeySID, 0, KV_SZ_LENGTH);
	memset(szValSID, 0, KV_SZ_LENGTH);
	memset(jsonDataBuf, 0, LPR_JSON_DATA_SIZE);

	/////////////////////////////////////////////////////////////
	/// Message example:
	/// 	DL: 200\r\n
	///		PT: JSON\r\n
	///		PC: NA\r\n
	///		SID: 99\r\n\r\n
	///		[Data...]
	int ret = sscanf(recvBuf, "%[^:]:%[^\r\n]\r\n%[^:]:%[^\r\n]\r\n%[^:]:%[^\r\n]\r\n%[^:]:%[^\r\n]\r\n",
		szKeyDL, szValDL, szKeyPT, szValPT, szKeyPC, szValPC, szKeySID, szValSID);

#if 0 ///-- DEBUG ONLY
	printf("\n=== Received data ... \n%s\n", recvBuf);
	printf("%s = %s, %s = %s, %s = %s, %s = %s \n", szKeyDL, szValDL, szKeyPT, szValPT, szKeyPC, szValPC, szKeySID, szValSID);
#endif

	if ( !(	(ret == 8) && STREQL("DL", szKeyDL) && STREQL("PT", szKeyPT) && STREQL("PC", szKeyPC) && STREQL("SID", szKeySID) ) ) {
		DBG("[ERROR][LPR_SRV] cmd parse error! ret = %d\n", ret);
		return;
	}

	char *headerEnd = NULL;
	int dlVal = atoi(szValDL);
	int sidVal = atoi(szValSID);

	_jsonSID = sidVal;

	TRACE("[LPR_SRV] DL=%d, PT=%s, PC=%s, SID=%d\n", dlVal, szValPT, szValPC, sidVal);

	cmdDataLen = dlVal;

	headerEnd = strstr(recvBuf, "\r\n\r\n");
	if (NULL==headerEnd) {
		DBG("[ERROR][LPR_SRV] can not find header end symbol (\\r\\n\\r\\n)");
		return;
	}

	TRACE( "[LPR_SRV] found header end symbol (\\r\\n\\r\\n)");
	headerEnd += 4;

	int dataSize = strlen(headerEnd);
	if (dataSize <= 0) {
		// no json data ?
		DBG("[FIXME] %s, %d !!\n", __FUNCTION__, __LINE__);
		return;
	}

	TRACE("copy json data len=%d", dataSize);
	memcpy(jsonDataBuf + jsonDataLen, headerEnd, dataSize);	/// why jsonDataBuf+jsonDataLen, shouldn't jsonDataLen be zero ???
	jsonDataLen += dataSize;

	// recv json data finish
	if (cmdDataLen == jsonDataLen) {
		FILE *pfile = NULL;

		pfile = fopen("/tmp/lpr_request.json", "w");
		if (pfile != NULL)	{
			if(jsonDataLen > 0) {
				int writeLen = 0;

				writeLen += fwrite(jsonDataBuf, 1, jsonDataLen, pfile);
				TRACE("[LPR_SRV] write len=%d\n", writeLen);
			}
			fclose(pfile);
		}
		TRACE("[LPR_SRV] recv json data finish, len=%d\n", jsonDataLen);

		TRACE("++++++++++ LPR_ProcessRequest() \n");
		LPR_ProcessRequest(jsonDataBuf, jsonDataLen, sidVal);

		//reset command parsing
		jsonDataLen = 0;
		findHeader = 0;
		_jsonSID = -1;
	}
	else if (cmdDataLen > jsonDataLen) {
		TRACE("[LPR_SRV] need to recv the reset of json datn\n");
	}
	else if (cmdDataLen < jsonDataLen) {
		DBG("[ERROR][LPR_SRV] json data length is more than DL length!, jsonDataLen=%d\n", jsonDataLen);
		//reset command parding
		jsonDataLen = 0;
		findHeader = 0;
		_jsonSID = -1;
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
	if (!findHeader) {
		DBG("[ERROR][LPR_SRV] recv json data without header found! len=%d\n", recv_len);
		return;
	}

	if ( (jsonDataLen + recv_len) >= LPR_JSON_DATA_SIZE ) {
		DBG("[ERROR][LPR_SRV] json data buffer overflow! jsonDataLen+recv_len=%d\n", jsonDataLen + recv_len);

		// reset command parsing
		jsonDataLen = 0;
		findHeader = 0;
		recvTimeoutCount = 0;

		return;
	}

	DBG("copy json data len=%d", recv_len);
	memcpy(jsonDataBuf + jsonDataLen, recvBuf, recv_len);
	jsonDataLen += recv_len;

	//recv jsondat finish
	if (cmdDataLen != jsonDataLen) {
		DBG("[ERROR][LPR_SRV] cmdDataLen != jsonDataLend !!\n");
		return;
	}

	FILE *pfile = NULL;
	pfile = fopen("/tmp/lpr_request.json", "w");
	if(pfile != NULL)
	{
		if(jsonDataLen > 0)
		{
			int writeLen = 0;
			writeLen += fwrite(jsonDataBuf, 1, jsonDataLen, pfile);
			DBG("[LPR_SRV] write len=%d\n", writeLen);

		}
		fclose(pfile);
	}
	DBG("[LPR_SRV] recv json data finish, len=%d\n", jsonDataLen);

	LPR_ProcessRequest(jsonDataBuf, jsonDataLen, _jsonSID);

	//reset command parsing
	jsonDataLen = 0;
	findHeader = 0;
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
	///////////////////////////////////////////////////////
	/// process incoming connection
	/// APP protocol: https://docs.google.com/document/d/1h8qM2tJFuIYJX0DSADmVdcMAKvoZXL3M/edit
	///////////////////////////////////////////////////////

	if (!FD_ISSET(g_clientSocket_FD, &fdsr)) {
		DBG("[WARNING][LPR_SRV] client recv but g_clientSocket_FD not SET!!??\n");
		return;
	}

	//////////////////////////////////////
	///-- receive message into buffer
	//////////////////////////////////////
	memset(recvBuf, 0, LPR_RECV_BUF_SIZE);
	int recv_len = recv(g_clientSocket_FD, (void*) recvBuf, LPR_RECV_BUF_SIZE, 0);    //recv data from app
	TRACE("received %d data from client - 0x%0X", recv_len, g_clientSocket_FD);
#if 0	///-- DEBUG ONLY
	if (recv_len) {
		printf("=== Received data ...\n %s\n\n", recvBuf);
		printf("=== Hex ... \n");
		int k;
		for (k=0; k<recv_len; k++) {
			printf("0x%02X ", recvBuf[k]);
		}
		puts("\n");
	}
#endif

	////////////////////////////////////
	/// recv() failed
	////////////////////////////////////
	if (recv_len < 0) {
		if(errno == EINTR) {
			DBG("[WARNING][LPR_SRV] Interrupted system call-- continue !! ");
			return;	/* Interrupted system call */
		}

		ERROR("[LPR_SRV] receive client data failed!!");
		close(g_clientSocket_FD);
		g_clientSocket_FD = -1;
		return;
	}

	////////////////////////////////////
	/// recv() : No data, session completed, client has disconnected
	////////////////////////////////////
	if (recv_len == 0) {
		// recv complete
		TRACE("[LPR_SRV] (recv_len == 0), client disconnected\n");
		close(g_clientSocket_FD);
		g_clientSocket_FD == -1;

		//reset command parsing
		findHeader = 0;
		jsonDataLen = 0;
		return;
	}

	////////////////////////////////////
	/// Start to process received message
	////////////////////////////////////

	if ( (!findHeader) && (recv_len > 3) && (recvBuf[0] == 'D') && (recvBuf[1] == 'L') && (recvBuf[2] == ':') ) {
		///--------------------------------------------
		/// (1) !findHeader --> the starting packet of message.
		///--------------------------------------------
		TRACE("processing starting packet ...");
		_process_starting_packet(recv_len);
	}
	else
	{
		///--------------------------------------------
		/// (2) findHeader --> rest packets of seession (except the starting one)
		///--------------------------------------------
		TRACE("processing trailing packets ...");
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
	// CY: recvBuf = NULL;
	// CY: use static buffer --  jsonDataBuf = NULL;
	clientSockfd = -1;
	jsonDataLen = 0;
	findHeader = 0;
	cmdDataLen = 0;


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
			debug_printf("Exiting thread loop ...(g_controlSVR_FD == -1) !!!\n");
			break;
		}

		if (g_clientSocket_FD > 0) {
			/// client connected -- skip select(g_controlSVR_FD), unnecessary 2sec timeout !!!
			goto _CHECK_CLIENT;
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
_CHECK_CLIENT:
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
			else if(ret == 0) {
				//--- timout
				if (findHeader || (jsonDataLen > 0)) {
					DBG("[LPR_SRV] recv data timeout! findHeader=%d, jsonDataLen=%d\n", findHeader, jsonDataLen);

					if (recvTimeoutCount >= 10) {
						//reset command paseing
						jsonDataLen = 0;
						findHeader = 0;

						DBG("[LPR_SRV] recv data timeout count=%d! reset findHeader\n", recvTimeoutCount);
					}
					else {
						recvTimeoutCount++;
					}
				}
			}
			else
			{
				///////////////////////////////////////////////////////
				/// process incoming connection
				/// APP protocol: https://docs.google.com/document/d/1h8qM2tJFuIYJX0DSADmVdcMAKvoZXL3M/edit
				///////////////////////////////////////////////////////
				_process_connection();
			}
			continue;
		}

		////////////////////////////////////////////////////
		// (3) Select(g_controlSVR_FD) found pending request !!
		////////////////////////////////////////////////////
		TRACE("[LPR_SRV] select(g_controlSVR_FD) ... accepting connection\n");
		_accept_connection();
	}	/// while()

	if(g_controlSVR_FD != -1) {
		close(g_controlSVR_FD);
		g_controlSVR_FD = -1;

		DBG("[FDFR_SRV] socket server is closed\n");
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
	debug_printf("creating controlSVR thread\n");
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

	debug_printf("controlSVR is created, g_controlSVR_FD = %d ...\n", g_controlSVR_FD);

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
	debug_printf("Stopping controlSVR ...");
	g_controlSVR_Running = 0;
	debug_printf(" completed!\n");
}



///------------------------------------------------------------------
/// @brief
/// @param
/// @return
/// @note
///------------------------------------------------------------------
void main(int argc, char **argv)
{
	//---------------------------------------------
	//-- Initialize global variables
	//---------------------------------------------

	//---------------------------------------------
	//-- Create socket for controlSVR to receive commands from mobile apps
	//---------------------------------------------
	DBG("Starting controlSVR ...\n");
	ControlSVR_Start();

	while(1) {
		sleep(10);	// usleep(1000);
#if 0 // Test Serial_Comm
		_serial_comm_set_display(_aqd_panel++);
		if (_aqd_panel > 9) _aqd_panel = 1;
		sleep(5);

		_serial_comm_get_air_value();
		sleep(5);
#endif
	}

	return;
}

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


//---------------------------------------------
//-- In-File Debug Printf
//---------------------------------------------
#define LOG_DBG		1
#define LOG_VERBOSE	0
#define LOG_FUNC	0

#define NULL_FUNCTION				do {} while(0)
#define error_printf(format, ...)   fprintf(stderr, "%s() @ %s, %d, ERROR: "format, __FUNCTION__, __FILE__, __LINE__, ##__VA_ARGS__);

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


static char _respJsonData[CSVR_RESP_BUF_MAX_SIZE] = {0};
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


static int _dewarp_mode;
static int _aqd_panel;	// air quality panel dispaly


///------------------------------------------------------------------
/// @brief
/// @param
/// @return
/// @note
///------------------------------------------------------------------
static int _ControlServerSendResult(char *respJsonBuf, unsigned int respJsonSize, char *bindata, long int binsize, int sid)
{
	extern int CSVR_ReplyResult(char *respJsonBuf, unsigned int respJsonSize, char *bindata, long int binsize, int sid);
	return CSVR_ReplyResult(respJsonBuf, respJsonSize, bindata, binsize, sid);

}

///!------------------------------------------------------
///! @brief		_Return_GetDewarp
///! @param
///! @return
///! @note
///!------------------------------------------------------
static int _Return_GetDewarp(int sid)
{
	sprintf(_respJsonData,
				"{"
				"\"cmd\": \"get_dewarp\","
				"\"result\": 0,"
				"\"dewarp1-1\": \"%d\"}",
				_dewarp_mode);
	verbose_printf("return GetAI:\n%s\n", _respJsonData);

	return _ControlServerSendResult(_respJsonData, strlen(_respJsonData), NULL, 0, sid);
}



///!------------------------------------------------------
///! @brief		_Return_SetDewarp
///! @param
///! @return
///! @note
///!------------------------------------------------------
static int _Return_SetDewarp(int result, int sid)
{
	char *jsonFormat = "{"
					   "\"cmd\": \"set_dewarp\","
					   "\"result\": %d"
					   "}";


	memset(_respJsonData, 0, sizeof(_respJsonData));
	sprintf(_respJsonData, jsonFormat, result);

	verbose_printf("return SetDewarp:\n%s\n", _respJsonData);

	return _ControlServerSendResult(_respJsonData, strlen(_respJsonData), NULL, 0, sid);
}



///!------------------------------------------------------
///! @brief		_Return_GetPanelDisplay
///! @param
///! @return
///! @note
///!------------------------------------------------------
static int _Return_GetPanelDisplay(int sid)
{
	sprintf(_respJsonData,
				"{"
				"\"cmd\": \"get_panel_display\","
				"\"result\": 0,"
				"\"display\": \"%d\"}",
				_aqd_panel);
	verbose_printf("return GetPanelDisplay:\n%s\n", _respJsonData);

	return _ControlServerSendResult(_respJsonData, strlen(_respJsonData), NULL, 0, sid);
}



///!------------------------------------------------------
///! @brief		_Return_SetPanel
///! @param
///! @return
///! @note
///!------------------------------------------------------
static int _Return_SetPanel(int result, int sid)
{
	char *jsonFormat = "{"
					   "\"cmd\": \"set_panel\","
					   "\"result\": %d"
					   "}";


	memset(_respJsonData, 0, sizeof(_respJsonData));
	sprintf(_respJsonData, jsonFormat, result);

	verbose_printf("return SetPanel:\n%s\n", _respJsonData);

	return _ControlServerSendResult(_respJsonData, strlen(_respJsonData), NULL, 0, sid);
}


#if 0
///!------------------------------------------------------
///! @brief		_Return_GetAirValue
///! @param
///! @return
///! @note
///!------------------------------------------------------
static int _Return_GetAirValue(int sid)
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
	verbose_printf("return GetAirValue:\n%s\n", _respJsonData);

	return _ControlServerSendResult(_respJsonData, strlen(_respJsonData), NULL, 0, sid);
}
#endif

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
///! @brief		WD360_ProcessRequest
///! @param
///! @return
///! @note
///!------------------------------------------------------
void WD360_ProcessRequest(char *recvJsonCmd, int length, int sid)
{
	char *cmdVal = NULL;
	int   cmdLen = 0;

	if (getJsonString(recvJsonCmd, "cmd", &cmdVal, &cmdLen) == 0) {
		verbose_printf("cmd: %s\n", cmdVal);

		if (STR_EQUAL(cmdVal, "get_dewarp")) {
			verbose_printf("get_dewarp\n");
			_Return_GetDewarp(sid);
		}
		else if (STR_EQUAL(cmdVal, "set_dewarp")) {
			verbose_printf("set_dewarp\n");
			char *dewarp_mode;
			int  sz_len;

			if ( 0 == getJsonString(recvJsonCmd, "dewarp1-1", &dewarp_mode, &sz_len) ) {
				int dwm = atoi(dewarp_mode);
				verbose_printf("dwm ---> %d\n", dwm);
				if (_validate_dewarp_mode(dwm) < 0) {
					// invalide dewarp mode
					_Return_SetDewarp(1 /*format error*/, sid);
				}
				else {
					_dewarp_mode = dwm;
					verbose_printf("_dewarp_mode = %d\n", _dewarp_mode);
					_Return_SetDewarp(0 /*success*/, sid);
				}
			}
			else {
				_Return_SetDewarp(1 /*format error*/, sid);
			}
			if (dewarp_mode) {	free(dewarp_mode); }
		}
		else if (STR_EQUAL(cmdVal, "get_panel_display")) {
			verbose_printf("get_panel_display\n");
			_Return_GetPanelDisplay(sid);
		}
		else if (STR_EQUAL(cmdVal, "set_panel")) {
			verbose_printf("set_panel\n");
			char *display;
			int  sz_len;

			if ( 0 == getJsonString(recvJsonCmd, "display", &display, &sz_len) ) {
				int aqd = atoi(display);
				if (aqd>0 && aqd<20) {
					_aqd_panel = aqd;
					verbose_printf(" _aqd_panel ---> %d\n", _aqd_panel);
					_Return_SetPanel(0 /*success*/, sid);
					#ifdef USE_SERIAL_COMM
					_serial_comm_set_display(_aqd_panel);
					#endif // USE_SERIAL_COMM
				}
				else {
					// invalide dewarp mode
					_Return_SetPanel(1 /*format error*/, sid);
				}
			}
			else {
				_Return_SetPanel(1 /*format error*/, sid);
			}
			if (display)	free(display);
		}
		#if 0
		else if (STR_EQUAL(cmdVal, "get_air_value")) {
			verbose_printf("get_air_value\n");
			#ifdef USE_SERIAL_COMM
			_serial_comm_get_air_value();
			#endif // USE_SERIAL_COMM
			_Return_GetAirValue(sid);
		}
		#endif
		else {
			error_printf("unsupported cmd: %s \n", cmdVal);
		}
	}

	//--------------------------------------------
	// !!! Don't forget to free cmdVal
	//--------------------------------------------
	if (cmdVal) {
		free(cmdVal);
	}
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <errno.h>
#include <pthread.h>
#include <signal.h>

#include <unistd.h>	// getopt

#include <stdbool.h>

#include <string.h>
#include <sys/sysinfo.h>

#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <errno.h>

#include "string_common.h"

#include "lpr_agent.h"

#define LOG_INFO	1
#define LOG_DBG		1
#define LOG_VERBOSE	1
#define LOG_FUNC	0
#include "infile_debug.h"


typedef struct found_plate_s {
	char			plate[LPR_PLATE_ID_LEN];
	char			name[LPR_PLATE_OWNER_LEN];
	time_t			tm;		// the lpr result is updated
	unsigned		bbox_x;
	unsigned		bbox_y;
	unsigned		bbox_w;
	unsigned		bbox_h;
	char			jpeg[LPR_JPEG_BUF_LEN];
	unsigned		jpeg_size;
} found_plate_t;

static found_plate_t _foundPlate;


#define JSON_RESP_DATA_LENGTH		(500*1024) 	// assume JPG or 2000 license plate data
static char _respJsonData[JSON_RESP_DATA_LENGTH] = {0};
#if 0
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
#endif

///!------------------------------------------------------
///! @brief
///! @param
///! @return
///! @note
///!------------------------------------------------------
static long int _load_binary_file(char *fpath, char **p)
{
	FILE *fp;
	char *buf;

	fp = fopen(fpath, "rb");
	if (fp == NULL) {
		error_printf("Failed to open %s !!\n", fpath);
		*p = NULL;
		return 0;
	}

	fseek(fp, 0L, SEEK_END);
	long int lsize = ftell(fp);
	buf = (char *) malloc(lsize);
	if (buf == NULL) {
		error_printf("Failed to allocate memeory to read file !!\n");
		*p = NULL;
		return 0;
	}

	fseek(fp, 0L, SEEK_SET);
	fread(buf, lsize, 1, fp);
	*p = buf;

	//printf(">>>> %x %x %x %x \n", buf[0], buf[1], buf[2], buf[3]);

	fclose(fp);
	return lsize;
}

///------------------------------------------------------------------
/// @brief
/// @param
/// @return
/// @note
///------------------------------------------------------------------
int LPR_SVC_init(void *data)
{
	memset(&_foundPlate, 0, sizeof(found_plate_t));
	_foundPlate.jpeg_size = 0;
}


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
///! @brief		LPR_Return_GetAI
///! @param
///! @return
///! @note
///!------------------------------------------------------
static int _Return_GetAI(int sid)
{
	#define KEY_PAIR_LEN	128
	static char keyPair[KEY_PAIR_LEN];	// char userItem[128] = {0};
	int first_key = 1;

	memset(keyPair, 0, KEY_PAIR_LEN);
	memset(_respJsonData, 0, sizeof(_respJsonData));
	sprintf(_respJsonData,
				"{"
				"\"cmd\": \"get_ai\","
				"\"function\": \"LPR\","
				"\"data\":[");

	int n_total;
	struct lpquery_list_s *qlist;
	n_total = lpr_queryALL_open(&qlist);
	struct lpquery_s *p = qlist->items;
	for (int i=0; i<n_total; i++) {
		//debug_printf(">>> license= %s, name= %s \n", p->license, p->name);
		if (first_key) {
			first_key = 0;
			sprintf(keyPair, "{\"license\":\"%s\",\"name\":\"%s\"}", p->license, p->name);
		}
		else {
			sprintf(keyPair, ",{\"license\":\"%s\",\"name\":\"%s\"}", p->license, p->name);
		}
		strcat(_respJsonData, keyPair);
		p++;
	}
	strcat(_respJsonData, "]}");

	lpr_queryALL_close();

	//verbose_printf("return GetAI:\n%s\n", _respJsonData);

	return _ControlServerSendResult(_respJsonData, strlen(_respJsonData), NULL, 0, sid);
}



///!------------------------------------------------------
///! @brief		_Return_SetAI
///! @param
///! @return
///! @note
///!------------------------------------------------------
static int _Return_SetAI(char *plate, char *name, int result, int sid)
{
	char *jsonFormat = "{"
					   "\"cmd\": \"set_ai\","
					   "\"function\": \"LPR\","
					   "\"result\": %d"
					   "}";

	//-- add/update license: name to database
	if (!result) {
		//result = pq_set(_db_obj, plate, name);
		result = lpr_data_set(plate, name);
		//result += pq_dump_file(_db_obj, DB_JSON_PATH);
		if (result) result = -1;
	}
	//printf("--- %s\n", pq_dumps(_db_obj));

	memset(_respJsonData, 0, sizeof(_respJsonData));
	sprintf(_respJsonData, jsonFormat, result);

	verbose_printf("return SetAI:\n%s\n", _respJsonData);

	return _ControlServerSendResult(_respJsonData, strlen(_respJsonData), NULL, 0, sid);
}



///!------------------------------------------------------
///! @brief		_Return_DeleteAI
///! @param
///! @return
///! @note
///!------------------------------------------------------
static int _Return_DeleteAI(char *plate, int result, int sid)
{
	char *jsonFormat = "{"
					   "\"cmd\": \"delete_ai\","
					   "\"function\": \"LPR\","
					   "\"result\": %d"
					   "}";

	//-- delete a license plate from database
	if (!result) {
		//result = json_object_del(_db_obj, plate);
		// result += pq_dump_file(_db_obj, DB_JSON_PATH);
		result = lpr_data_delete(plate);
		if (result) result = -1;
	}

	memset(_respJsonData, 0, sizeof(_respJsonData));
	sprintf(_respJsonData, jsonFormat, result);

	verbose_printf("return DeleteAI: %s\n", _respJsonData);

	return _ControlServerSendResult(_respJsonData, strlen(_respJsonData), NULL, 0, sid);
}



///!------------------------------------------------------
///! @brief		_Return_UpdateAI
///! @param
///! @return
///! @note
///!------------------------------------------------------
static int _Return_UpdateAI(char *plate, char *name, int result, int sid)
{
	char *jsonFormat = "{"
					   "\"cmd\": \"update_ai\","
					   "\"function\": \"LPR\","
					   "\"result\": %d"
					   "}";

	//-- update a license plate in database
	if (!result) {
		// result = json_object_update_existing(_db_obj, json_pack("{ss}", plate, name));
		// result += pq_dump_file(_db_obj, DB_JSON_PATH);
		result = lpr_data_update_existing(plate, name);
		if (result) result = -1;
	}

	//printf("UpdateAI: %s:%s ******* %d \n", plate, name, result);

	memset(_respJsonData, 0, sizeof(_respJsonData));
	sprintf(_respJsonData, jsonFormat, result);

	verbose_printf("return UpdateAI: %s\n", _respJsonData);

	return _ControlServerSendResult(_respJsonData, strlen(_respJsonData), NULL, 0, sid);
}



///!------------------------------------------------------
///! @brief		LPR_Return_GetLPR
///! @param
///! @return
///! @note
///!------------------------------------------------------
static int _Return_GetLPR(found_plate_t *plate, int result, int sid)
{
	char *jsonFormat_no_plate = "{"
					   "\"cmd\": \"get_lpr\","
					   "\"result\": %d"
					   "}";

	char *jsonFormat = "{"
					   "\"cmd\": \"get_lpr\","
					   "\"result\": %d,"
					   "\"lpid\": \"%s\","
					   "\"owner\": \"%s\","
					   "\"x\": %d,"
					   "\"y\": %d,"
					   "\"w\": %d,"
					   "\"h\": %d"
					   "}\r\n\r\n";

	memset(_respJsonData, 0, sizeof(_respJsonData));
	unsigned int _json_size;
	if (result) {
		//-- no plate
		result = 34;
		sprintf(_respJsonData, jsonFormat_no_plate, result);
		_json_size = strlen(_respJsonData);
	}
	else {
		//-- found valid plate
		result = 0;
		sprintf(_respJsonData, jsonFormat,
				result,
				plate->plate, plate->name,
				plate->bbox_x, plate->bbox_y, plate->bbox_w, plate->bbox_h);
		_json_size = strlen(_respJsonData);

		#if 0 // DEBUG ONLY
		do {
			unsigned char *p;
			printf("[Before copy, size=%d @ %p] %s\n", _json_size, _respJsonData, _respJsonData);
			p = plate->jpeg;
			printf("[copy JPEG, size=%d, %x %x %x %x \n", plate->jpeg_size, p[0], p[1], p[2], p[3] );
		} while(0);
		#endif

		#if 0 // DEBUG ONLY
		do {
			unsigned char *p;
			p = (unsigned char *)&_respJsonData[_json_size];
			printf("[After copy, size=%d %x %x %x %x \n", _json_size, p[0], p[1], p[2], p[3]);
		} while(0);
		#endif
	}

	info_printf("return GetLPR:\n%s\n", _respJsonData);

	return _ControlServerSendResult(_respJsonData, _json_size, plate->jpeg, plate->jpeg_size, sid);
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
		debug_printf("[LPR_SRV] no \"%s\" in jason\n", key);
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
		//    debug_printf("[LPR_SRV] error_printf \"%s\" value in json\n", key);
		//    return -1;
		//}
	}

	keyBufSize = i + 1;
	keyBuf = malloc(keyBufSize);
	if (keyBuf == NULL) {
		debug_printf("[LPR_SRV] malloc error_printf!\n");
		return -1;
	}

	memset(keyBuf, 0, keyBufSize);
	memcpy(keyBuf, keyNamePtr, keyBufSize - 1);

	//debug_printf("[LPR_SRV] find key: %s, size=%d\n", keyBuf, keyBufSize);

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
			debug_printf("[LPR_SRV] malloc error_printf! valBufSize=%d\n", valBufSize);
			ret = -1;
			goto FUNC_RETURN;
		}

		memset(*value, 0, valBufSize);
		memcpy(*value, keyBuf + first_doublequotes_pos + 1, valBufSize - 1);
		*length = valBufSize - 1;

		//debug_printf( "[LPR_SRV] colon_pos=%d, first_dq_pos=%d, second_dq_pos=%d", colon_pos, first_doublequotes_pos, second_doublequotes_pos);
		if (*length < 32) {
			//debug_printf("[LPR_SRV] getJsonString() value=%s", *value);
		}
		//debug_printf("[LPR_SRV] getJsonString() length=%d", *length);
	}
	else {
		error_printf("[LPR_SRV] getJsonString() fail to get string value of %s\n!", keyName);
	}

FUNC_RETURN:

	if (keyBuf) {
		free(keyBuf);
	}

	return ret;
}



// typedef struct conx_cb_ {
// 	char recvBuf[LPR_JSON_DATA_SIZE];		//char *recvBuf;
// 	char jsonDataBuf[LPR_JSON_DATA_SIZE];	// char *jsonDataBuf;
// 	fd_set fdsr;
// 	// struct timeval timeout;
// 	struct sockaddr_in clientInfo;
// 	int addrlen;
// //		int ret;
// 	int clientSockfd;
// 	int jsonDataLen;
// 	int findHeader;
// 	int recvTimeoutCount;
// 	int cmdDataLen;
// } conx_cb_t;

// static conx_cb_t conx_cb;

// #define recvBuf				(conx_cb.recvBuf)
// #define jsonDataBuf			(conx_cb.jsonDataBuf)
// #define fdsr				(conx_cb.fdsr)
// // #define timeout				(conx_cb.timeout)
// #define clientInfo			(conx_cb.clientInfo)
// #define addrlen				(conx_cb.addrlen)
// #define clientSockfd		(conx_cb.clientSockfd)
// #define jsonDataLen			(conx_cb.jsonDataLen)
// #define findHeader			(conx_cb.findHeader)
// #define recvTimeoutCount	(conx_cb.recvTimeoutCount)
// #define cmdDataLen			(conx_cb.cmdDataLen)


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

	if (getJsonString(jsonData, "cmd", &cmdVal, &cmdLen) == 0) {
		info_printf("[%s] \"cmd\": %s, sid=%d\n", __FUNCTION__, cmdVal, sid);

		if (STR_EQUAL(cmdVal, "get_ai")) {
			debug_printf(">>> get_ai\n");
			_Return_GetAI(sid);
		}
		else if (STR_EQUAL(cmdVal, "set_ai")) {
  			debug_printf(">>> set_ai\n");
			char *name, *plate;
			int  sz_len;

			if ( 0 == getJsonString(jsonData, "license", &plate, &sz_len) ) {
				if ( 0 == getJsonString(jsonData, "name", &name, &sz_len) ) {
					_Return_SetAI(plate, name, 0 /*success*/, sid);
				}
			}
			else {
				_Return_SetAI("xxxx", "????", 2 /*format error_printf*/, sid);
			}
			if (name)	free(name);
			if (plate)	free(plate);
		}
		else if (STR_EQUAL(cmdVal, "delete_ai")) {
			debug_printf(">>> delete_ai\n");
			char *plate;
			int	 sz_len;

			if (getJsonString(jsonData, "license", &plate, &sz_len)==0) {
				_Return_DeleteAI(plate, 0 /*success*/, sid);
			}
			else {
				_Return_DeleteAI(plate, 2 /*format error_printf*/, sid);
			}
			if (plate)	free(plate);
		}
		else if (STR_EQUAL(cmdVal, "update_ai")) {
			debug_printf(">>> update_ai\n");
			char *name, *plate;
			int	 sz_len;

			if ( 0 == getJsonString(jsonData, "license", &plate, &sz_len) ) {
				if ( 0 == getJsonString(jsonData, "name", &name, &sz_len) ) {
					_Return_UpdateAI(plate, name, 0 /*success*/, sid);
				}
			}
			else {
				_Return_UpdateAI("xxxx", "????", 2 /*format error_printf*/, sid);
			}
			if (name)	free(name);
			if (plate)	free(plate);
		}
		else if (STR_EQUAL(cmdVal, "get_lpr")) {
			info_printf(">>> get_lpr\n");

			int isUpdated = 0;
			LOCK_LPR_RESULT;
			if (lpr_result->is_updated) {
				snprintf(_foundPlate.plate, LPR_PLATE_ID_LEN, "%s", lpr_result->plate);
				snprintf(_foundPlate.name, LPR_PLATE_OWNER_LEN, "%s", lpr_result->name);
				_foundPlate.bbox_x = lpr_result->bbox_x;
				_foundPlate.bbox_y = lpr_result->bbox_y;
				_foundPlate.bbox_w = lpr_result->bbox_w;
				_foundPlate.bbox_h = lpr_result->bbox_h;
				unsigned jsize = (lpr_result->jpeg_size < LPR_JPEG_BUF_LEN) ? lpr_result->jpeg_size : LPR_JPEG_BUF_LEN;
				memcpy(_foundPlate.jpeg, lpr_result->jpeg_buf, jsize);
				_foundPlate.jpeg_size = jsize;

				_foundPlate.tm = lpr_result->tm;
				lpr_result->is_updated = 0;

				isUpdated = 1;
			}
			UNLOCK_LPR_RESULT;

			if (isUpdated) {
				//-- car enterance
				_Return_GetLPR(&_foundPlate, 0 /*success*/, sid);
			}
			else {
				//-- No car enterance
				_Return_GetLPR(&_foundPlate, 34 /*no plate*/, sid);
			}

		printf("---3---\n");
		}
		else {
			debug_printf("[LPR_SRV] unsupported cmd: %s", cmdVal);
		}
	}

	//--------------------------------------------
	// !!! Don't forget to free cmdVal
	//--------------------------------------------
	if (cmdVal) {
		free(cmdVal);
		info_printf("[LPR_SRV] free cmdVal\n");
	}
}

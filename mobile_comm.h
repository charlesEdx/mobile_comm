#ifndef _MOBILE_COMM_H_
#define _MOBILE_COMM_H_


#include "string_common.h"
#include "json_common.h"


#ifdef __cplusplus
extern "C" {
#endif

#if 0
#define MC_SAVE_JSON_CMD_TO_FILE
#define MC_CMD_SAVE_JSON_PATH		"/tmp/mc_cmd_save.json"
#endif

// #define CREATE_COMMAND_MSGQ
// #define CREATE_NOTIFY_MSGQ

//-- [PORT] define port number of the controlSVR socket
#define CONTROL_SVR_PORT		8481

//---[PORT] Retry interval while controlSVR socket select() errors, in second
#define CSVR_SELECT_ERROR_RETRY_INTVAL			5		// in Second

//--- [PORT] Timeout interval of checking controlSVR socket status (select()),  in millisecond
#define CSVR_SELECT_TIMEOUT_INTVAL				1000	// in millisecond

//--- [PORT] Dwell/sleep time interval (in millisecond) while socket select() timeout, no wait if ZERO
#define CSVR_SELECT_TIMEOUT_DWELL_INTVAL		100		// in millsecond

//-- [PORT] timeout count threshold to abort connected session
#define CSVR_CONNECTION_ABORT_TIMEOUT_COUNT		10

//-- [PORT] define the buffer size which is to store received socket data from mobile app
#define CSVR_RECV_CMDBUF_SIZE 					(32<<10)	// in KBytes

//-- [PORT] define the buffer to store whole json cmd from app, it should be larger than CSVR_RECV_CMDBUF_SIZE
#define CSVR_RECV_JSON_CMD_SIZE 				(300<<10) // in KBytes

//--- [PORT] configure response buffer - the response buffer consists of (1)JSON Header (2)JSON Data (3)Binary Data (e.g. JPEG image)
#define CSVR_RESP_JSON_HEADER_MAX_SIZE			(256)
#define CSVR_RESP_JSON_DATA_MAX_SIZE			(32<<10)
#define CSVR_RESP_BIN_DATA_MAX_SIZE				(500<<10)
#define CSVR_RESP_BUF_MAX_SIZE					(CSVR_RESP_JSON_HEADER_MAX_SIZE+CSVR_RESP_JSON_DATA_MAX_SIZE+CSVR_RESP_BIN_DATA_MAX_SIZE)

#ifdef TEST_RELEASE
// for development
#define DB_TXT_PATH "/mnt/nfs/lpr_db.txt"
#define DB_JSON_PATH "/mnt/nfs/lpr_db.json"
#define RTSP_SET_FILE "/mnt/nfs/rtsp_set.json"
#else
#define DB_TXT_PATH "/mnt/SD0/lpr_db.txt"
#define DB_JSON_PATH "/mnt/SD0/lpr_db.json"
#define RTSP_SET_FILE "/mnt/mtd/rtsp_set.json"
#endif



#ifdef __cplusplus
}
#endif

#endif  // _MOBILE_COMM_H_

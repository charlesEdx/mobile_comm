#ifndef _MOBILE_COMM_H_
#define _MOBILE_COMM_H_

#ifdef __cplusplus
extern "C"
{
#endif




#define CREATE_COMMAND_MSGQ
#define CREATE_NOTIFY_MSGQ

//-- [PORT] define port number of the controlSVR socket
#define CONTROL_SVR_PORT		8481

//---[PORT] Retry interval while controlSVR socket select() errors, in second
#define CSVR_SELECT_ERROR_RETRY_INTVAL		5		// in Second

//--- [PORT] Timeout interval of checking controlSVR socket status (select()),  in millisecond
#define CSVR_SELECT_TIMEOUT_INTVAL			500		// in millisecond

//--- [PORT] Dwell/sleep time interval (in millisecond) while socket select() timeout, no wait if ZERO
#define CSVR_SELECT_TIMEOUT_DWELL_INTVAL	100		// in millsecond





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

#include <stdio.h>
#include <unistd.h>
// #include <jpeglib.h>
// #include <errno.h>
// #include <pthread.h>
// #include <signal.h>

#include <jansson.h>
// #include <stdbool.h>

#include <string.h>
#include <strings.h>
#include <sys/sysinfo.h>

// #include <sys/time.h>
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <time.h>
// #include <errno.h>

// #include "mobile_comm.h"
// #include "mobile_comm_private.h"
// #include "mc_common.h"


//---------------------------------------------
//-- In-File Debug Printf
//---------------------------------------------
#define LOG_DBG		0
#define LOG_VERBOSE	0

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
		debug_printf("no \"%s\" in jason\n", key);
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
	}

	keyBufSize = i + 1;
	keyBuf = malloc(keyBufSize);
	if (keyBuf == NULL) {
		error_printf("malloc error!\n");
		return -1;
	}

	memset(keyBuf, 0, keyBufSize);
	memcpy(keyBuf, keyNamePtr, keyBufSize - 1);

	//verbose_printf("find key: %s, size=%d\n", keyBuf, keyBufSize);

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
			error_printf("malloc error! valBufSize=%d \n", valBufSize);
			ret = -1;
			goto FUNC_RETURN;
		}

		memset(*value, 0, valBufSize);
		memcpy(*value, keyBuf + first_doublequotes_pos + 1, valBufSize - 1);
		*length = valBufSize - 1;

		//verbose_printf( "colon_pos=%d, first_dq_pos=%d, second_dq_pos=%d", colon_pos, first_doublequotes_pos, second_doublequotes_pos);
		if (*length < 32) {
			//verbose_printf("getJsonString() value=%s \n", *value);
		}
		//verbose_printf("getJsonString() length=%d \n", *length);
	}
	else {
		error_printf("getJsonString() fail to get string value of %s \n!", keyName);
	}

FUNC_RETURN:

	if (keyBuf) {
		free(keyBuf);
	}

	return ret;
}


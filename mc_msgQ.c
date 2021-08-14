#include <stdio.h>
#include <unistd.h>

#include <errno.h>
#include <pthread.h>
#include <signal.h>

// #include <jansson.h>
// #include <stdbool.h>

// #include <string.h>
// #include <strings.h>
// #include <sys/sysinfo.h>

// #include <sys/time.h>
// #include <sys/socket.h>
// #include <netinet/in.h>

#include <time.h>

#include "mobile_comm.h"
// #include "mc_common.h"


#ifdef USE_SERIAL_COMM

//#include "serial_comm.h"

static char _prog_version[] = "0.00.30";
static int _comm2host_Q, _host2comm_Q;
static serial_comm_msg_t _host2comm_msg, _comm2host_msg;

#define MCU_CMD_MAX_SIZE	64
static char _mcu_cmd[MCU_CMD_MAX_SIZE];

#endif	// USE_SERIAL_COMM


///--------------------------------------------------------------------------------------
/// @brief    :
/// @param    :
/// @return   :
/// @details  :
///--------------------------------------------------------------------------------------
// static int _msgQ_flush(int msgQ_id, long mtype, serial_comm_msg_t *msg, int max_flush)
// {
// 	int timeout = (!max_flush) ? 0 : 1;
// 	while(1) {
// 		if (msgrcv(msgQ_id, msg, SERIAL_COMM_MSG_DATBUF_SIZE, mtype, IPC_NOWAIT)<=0) {
// 			//-- no msg in queue
// 			break;
// 		}

// 		if (!timeout) continue;

// 		if (--max_flush<0)
// 			break;
// 	}
// }


#ifdef CREATE_COMMAND_MSGQ
static int _mc_cmd_Q;

///--------------------------------------------------------------------------------------
/// @brief    : To create msgQ for command
/// @param    :
/// @return   : SUCCESS=0, FAILURE=-1
/// @details  :
///--------------------------------------------------------------------------------------
static int _mc_cmdQ_init(void)
{
	printf("creating mobile_comm cmdQ with key: 0x%x ...\n", MC_CMD_Q_KEY);
	if ((_mc_cmd_Q=msgget(MC_CMD_Q_KEY, IPC_CREAT)==-1)) {
		perror("creating _mc_cmd_Q");
		return -1;
	}

	return 0;
}
#endif


#ifdef CREATE_NOTIFY_MSGQ
static int _mc_noti_Q;

static int _mc_notiQ_init(void)
{
	printf("creating mobile_comm notiQ with key: 0x%x ...\n", MC_NOTI_Q_KEY);
	if ((_mc_noti_Q=msgget(MC_NOTI_Q_KEY, IPC_CREAT)==-1)) {
		perror("creating _mc_noti_Q");
		return -1;
	}

	return 0;
}
#endif

#if 0
///--------------------------------------------------------------------------------------
/// @brief    : Send message to designated Q
/// @param    :
/// @return   :
/// @details  :
///--------------------------------------------------------------------------------------
static int _msgQ_send(int msgQ_id, long mtype, char *data, int size)
{
	if (!data || size > SERIAL_COMM_MSG_DATBUF_SIZE) {
		return -1;
	}

	serial_comm_msg_t *msg = (serial_comm_msg_t *)malloc(sizeof(serial_comm_msg_t));
	if (!msg) {
		return -1;
	}

	char *pdatbuf;
	msg->mtype = mtype;
	pdatbuf = msg->datbuf;
	memset(pdatbuf, 0, SERIAL_COMM_MSG_DATBUF_SIZE);
	memcpy(pdatbuf, data, size);

	msgsnd(msgQ_id, msg, SERIAL_COMM_MSG_DATBUF_SIZE, IPC_NOWAIT);

	free(msg);

	return 0;
}


///--------------------------------------------------------------------------------------
/// @brief    :
/// @param    :
/// @return   :
/// @details  :
///--------------------------------------------------------------------------------------
static int _msgQ_receive(int msgQ_id, long mtype, serial_comm_msg_t *msg)
{
	if (msgrcv(msgQ_id, msg, SERIAL_COMM_MSG_DATBUF_SIZE, mtype, 0) < 0) {
		printf("_msgQ_receive: msgQ(%d) %s\n", msgQ_id, strerror(errno));
		return -1;
	}

	return 0;
}


///--------------------------------------------------------------------------------------
/// @brief    : host to send msg to serial_comm
/// @param    :
/// @return   :
/// @details  :
///--------------------------------------------------------------------------------------
static int _host2commQ_send(char *data, int size)
{
	return _msgQ_send(_host2comm_Q, HOST2COMM_MTYPE, data, size);
}


///--------------------------------------------------------------------------------------
/// @brief    : host to receive msg from serial_comm
/// @param    :
/// @return   :
/// @details  :
///--------------------------------------------------------------------------------------
static int _comm2hostQ_flush(void)
{
	_msgQ_flush(_comm2host_Q, COMM2HOST_MTYPE, &_comm2host_msg, 100);
	return 0;
}


///--------------------------------------------------------------------------------------
/// @brief    : host to receive msg from serial_comm
/// @param    :
/// @return   :
/// @details  :
///--------------------------------------------------------------------------------------
static int _comm2hostQ_receive(char *data, int size)
{
	if (_msgQ_receive(_comm2host_Q, COMM2HOST_MTYPE, &_comm2host_msg) < 0) {
		printf("comm2hostQ_receive: failed!\n");
		return -1;
	}

	int copy_size;
	copy_size = (size > SERIAL_COMM_MSG_DATBUF_SIZE) ? SERIAL_COMM_MSG_DATBUF_SIZE : size;
	memcpy(data, _comm2host_msg.datbuf, copy_size);

	return copy_size;
}
#endif

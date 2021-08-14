#ifndef _COMM_MSGQ_H_
#define _COMM_MSGQ_H_

#include <stdio.h>
#include <string.h>

// Linux headers
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()

#include <pthread.h>

#include <stdlib.h>
#include <sys/msg.h>

#ifdef __cplusplus
extern "C" {
#endif


#define MSG_FTOK_FILE		"/etc/passwd"
#define HOST2COMM_MTYPE		100
#define COMM2HOST_MTYPE		200
#define HOST2COMM_Q_KEY		ftok(MSG_FTOK_FILE, HOST2COMM_MTYPE)
#define COMM2HOST_Q_KEY		ftok(MSG_FTOK_FILE, COMM2HOST_MTYPE)


#define SERIAL_COMM_MSG_DATBUF_SIZE	2048
typedef struct serial_comm_msg_s {
	long	mtype;			// type of message
	char	datbuf[SERIAL_COMM_MSG_DATBUF_SIZE];	// buffer of the messae data
} serial_comm_msg_t;




#ifdef __cplusplus
}
#endif

#endif  // _COMM_MSGQ_H_

#ifndef _MC_MSGQ_H_
#define _MC_MSGQ_H_

#ifdef __cplusplus
extern "C" {
#endif

//----- [PORT] You might need to choose a file path to generate message Keys
#define MSG_FTOK_FILE		"/etc/passwd"

#define MC_CMD_MTYPE		100
#define MC_ACK_MTYPE		101
#define MC_NOTI_MTYPE		200

#define MC_CMD_Q_KEY		ftok(MSG_FTOK_FILE, MC_CMD_MTYPE)
#define MC_NOTI_Q_KEY		ftok(MSG_FTOK_FILE, MC_NOTI_MTYPE)

// #define SERIAL_COMM_MSG_DATBUF_SIZE	2048
// typedef struct serial_comm_msg_s {
// 	long	mtype;			// type of message
// 	char	datbuf[SERIAL_COMM_MSG_DATBUF_SIZE];	// buffer of the messae data
// } serial_comm_msg_t;


static int _mc_cmdQ_init(void);
static int _mc_notiQ_init(void);


#ifdef __cplusplus
}
#endif

#endif  // _MC_MSGQ_H_

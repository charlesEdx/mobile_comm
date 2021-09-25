#ifndef PROG_COMMON_H_
#define PROG_COMMON_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#ifdef __cplusplus
extern "C"
{
#endif

// #define TEST_RELEASE

// #define SAVE_RGB888_IMAGE
// #define SAVE_RGB565_IMAGE

// #define MAX_IMG_WIDTH		1280
// #define MAX_IMG_HEIGHT		720
// #define MAX_RGBA_SIZE		(MAX_IMG_WIDTH * MAX_IMG_HEIGHT * 4)
// #define MAX_RGB565_SIZE		(MAX_IMG_WIDTH * MAX_IMG_HEIGHT * 2)

//------------------------------------------------------------------------------
// General Macros
//------------------------------------------------------------------------------
#define STREQL(s1, s2)      		(!strcmp(s1, s2))
#define SNPRINTF(sz ,FMT, ARG...)   snprintf(sz, sizeof(sz), FMT, ## ARG)

// #define	DBG(FMT, ARG...)		fprintf(stderr, "\033[0;36m[" __FILE__ ":%d] " FMT "\033[0m\n", __LINE__, ## ARG)
// #define	TRACE(FMT, ARG...)		; //fprintf(stderr, "\033[0;33m[" __FILE__ ":%d] " FMT "\033[0m\n", __LINE__, ## ARG)
// #define	ERROR(FMT, ARG...)		fprintf(stderr, "\033[0;31m[" __FILE__ ":%d] [ERROR]" FMT "\033[0m\n", __LINE__, ## ARG)


//get snapshot
#define RTSP_SET_FILE "/mnt/mtd/rtsp_set.json"
struct auth_info {
	char uri[128];
	int httpport;
	char addr[128];
	char path[128];
	char admin_name[128];
	char admin_pass[128];
};
enum url_type {
	URL_BAD,
	URL_NORMAL,
	URL_OLD_TFTP,
	URL_PREFIX
};
struct url_info {
	char *scheme;
	char *user;
	char *passwd;
	char *host;
	unsigned int port;
	char *path;			/* Includes query */
	enum url_type type;
};

#ifndef U8
#define U8   unsigned char
#endif


int RUN_SYSTEM_CMD(const char *cmd);
// int initDB(char *db_txt_path, char *db_json_path, path_query_obj *db_obj);

int load_image_from_file(char *fn, char *img_buf, unsigned int *img_size);


#ifdef __cplusplus
}
#endif

#endif  // PROG_COMMON_H_

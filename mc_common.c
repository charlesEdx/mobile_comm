#include <stdio.h>
#include <unistd.h>

#include <errno.h>
#include <pthread.h>
#include <signal.h>

#include "path_query.h"
#include <stdbool.h>

#include <string.h>
#include <sys/sysinfo.h>
#include "mc_common.h"

///--------------------------------------------------------------------------------------
/// @brief    :
/// @param    :
/// @return   :
/// @details  :
///--------------------------------------------------------------------------------------
int RUN_SYSTEM_CMD(const char *cmd)
{
	int ret;

	DBG("Running system call - %s", cmd);
	ret = system(cmd);
	return ret;
}

#if 0
int32_t ft_Realloc_Memory(uint8_t **pMem, uint32_t *nBufferSize, uint32_t nNewSize)
{
	int32_t nRtn = PT_ERR_SUCCESS;
	do {
		if (*nBufferSize > 0) {
			uint8_t* newBuffer;

			if ((newBuffer = (uint8_t*)realloc(*pMem, nNewSize)) == NULL) {
				DBG_LOG(PT_LOG_LV_ERR, "%s-%d->Realloc memory failed\n", __FILE__, __LINE__);
				nRtn = PT_ERR_MEMORY;
				break;
			}
			*pMem = newBuffer;
			*nBufferSize = nNewSize;
		}
		else {
			*nBufferSize = nNewSize;
			*pMem = (uint8_t*)malloc(*nBufferSize);
		}
	} while (0);

	return nRtn;
}




// CY modified: Load json db first, then txt db.
///!------------------------------------------------------
///! @brief		initDB
///! @param		db_txt_path		string of the LP db txt file
///! @param		db_json_path	string of the LP db json file
///! @param		[out] db_obj	pointer to store loaded json database
///! @return
///! @note
///!------------------------------------------------------
int initDB(char *db_txt_path, char *db_json_path, path_query_obj *db_obj)
{
	FILE *pLPRFile = NULL;
	// char *string;
	path_query_obj obj = NULL, lpr_obj = NULL;
	char lp_num[16], lp_name[64];
	unsigned init_json_load = 0;

	DBG("Initializing DB ... %s\n", db_json_path);
	///--- check json DB file first.
	if (!access(db_json_path, R_OK)) {
		DBG("Json DB exist, going to load from json DB\n");
		init_json_load = 1;
		goto _load_json_DB;
	}
	///--- if no existing json DB, check txt DB

_load_txt_DB:
	///-- load txt DB
	DBG("Loading txt DB ... %s\n", db_txt_path);
	if ((access(db_txt_path, F_OK))==-1) {
		ERROR("Neither josn nor txt LPR DB found!!\n");
		return -1;
	}

	pLPRFile = fopen(db_txt_path, "r");
	if ( NULL == pLPRFile) {
		ERROR("open LPR DB file fail!! \n");
		return -1;
	}

_load_json_DB:
	DBG("Loading json DB ...\n");
	obj = json_load_file(db_json_path, JSON_DECODE_ANY, NULL);

	if (!obj) {
		DBG("NULL json DB obj !!\n");

		if (init_json_load) {
			///-- go and check txt DB
			//printf("[Warning] No JSON LPR DB found, check txt DB...\n");
			init_json_load = 0;
			goto _load_txt_DB;
		}

		/// You are here because the case:
		/// --- found json DB but empty json DB, and a txt DB found and loaded
		/// --- we can establish the database from txt DB
		obj = pq_new();

		DBG("Establishing DB from txt data...\n");
		while(!feof(pLPRFile)) {
			char line_sz[128], *p_rst;

			p_rst = fgets(line_sz, 128, pLPRFile);
			if (!p_rst) {
				break;
			}
			//printf("lineSZ= %s\n", line_sz);
			sscanf(line_sz, "%s\t%[^\n]", lp_num, lp_name);
			//printf("---> LP_data= %s, lp_name= %s\n", lp_num, lp_name);
			pq_set(obj, lp_num, lp_name);
		}
	}
	// string = pq_dumps(obj);
	//DBG("2 LPR number =%s \n",string);
	pq_dump_file(obj, db_json_path);
	*db_obj = obj;
	// pq_free(obj);

	if (pLPRFile) {
		fclose(pLPRFile);
	}

	return 0;
}


///!------------------------------------------------------
///! @brief		add_LRPtoDB
///! @param
///! @return
///! @note
///!------------------------------------------------------
char add_LRPtoDB(char *LPR_NUM, char *db_json_path)
{
	path_query_obj obj = NULL;
	char *string;
	DBG("Add %s to %s \n",LPR_NUM,db_json_path);
	obj = json_load_file(db_json_path, JSON_DECODE_ANY, NULL);
	if(pq_set(obj, LPR_NUM, "1")!= 0)
	{
		DBG("add lpr num to DB fail\n");
		return -1;
	}
	string = pq_dumps(obj);
	DBG("ad LPR NUM = %s\n",string);
	pq_dump_file(obj, db_json_path);
	pq_free(obj);

	return 0;
}

///--------------------------------------------------------------------------------------
/// @brief    :
/// @param    :
/// @return   :
/// @details  :
///--------------------------------------------------------------------------------------
char delete_LRPfromDB(char *LPR_NUM, char *db_json_path)
{
	path_query_obj obj = NULL;
	char *string;
	DBG("Delete %s from DB %s\n",LPR_NUM, db_json_path);
	obj = json_load_file(db_json_path, JSON_DECODE_ANY, NULL);
	if(json_object_del(obj, LPR_NUM)!= 0)
	{
		DBG("delete lpr num from DB fail\n");
		return -1;
	}
	string = pq_dumps(obj);
	DBG("del LPR NUM = %s\n",string);
	pq_dump_file(obj, db_json_path);
	pq_free(obj);

	return 0;
}


///!------------------------------------------------------
///! @brief
///! @param
///! @return
///! @note
///!------------------------------------------------------
char check_LRPinDB(char *LPR_NUM, char *db_json_path)
{
	path_query_obj obj = NULL;
	char value[128];

	obj = json_load_file(db_json_path, JSON_DECODE_ANY, NULL);
	if(pq_get(obj, LPR_NUM, value ,sizeof(value),NULL) == NULL)
	{
		DBG("License : %s not in DB\n", LPR_NUM);
		return -1;
	}
	else
	{
		DBG("License %s is OK notify camera to open gate\n",LPR_NUM);
	}

	pq_free(obj);

	return 0;
}
#endif

#if 0	//////////////////////
///--------------------------------------------------------------------------------------
/// @brief    : load_image_from_file
/// @param    : fn			[in] the image file name
/// @param	  : img_buf		[in] the buffer to store the image data
/// @param	  : img_size	[in] max size of the img_buf [out] the real size of the image
/// @return   :
/// @details  :
///--------------------------------------------------------------------------------------
int load_image_from_file(char *fn, char *img_buf, unsigned int *img_size)
{
	if (NULL == fn) {
		printf("file name is null!\n");
		return -1;
	}

	FILE* in = fopen(fn, "rb");
	if (!in) {
		printf("opening file %s failed, error:%s.!\n", fn, strerror(errno));
		return -1;
	}

	fseek(in, 0, SEEK_END);
	long size = ftell(in);  //get the size

	if ( ((long) img_size) < size ) {
		printf("[ERROR] insufficient space to load image!!\n");
		return -1;
	}

	memcpy(img_buf, 0, *img_size);
	fseek(in, 0, SEEK_SET);  //move to begining
	fread(img_buf, 1, size, in);

	*img_size = (unsigned int)size;

	fclose(in);

	return 0;
}
#endif	//////////////////////////////////


#if 0
void main(void)
{
	char check_data[9] = "ANY-7788";
	char add_data[9] = "ANY-4588";
	char delete_data[9] = "ANY-1234";
	initDB(DB_TXT_PATH, DB_JSON_PATH);
	//test function
	add_LRPtoDB(add_data, DB_JSON_PATH);

	check_LRPinDB(check_data, DB_JSON_PATH);

	delete_LRPfromDB(delete_data, DB_JSON_PATH);
#if 0
	path_query_obj obj = NULL;
	obj = json_load_file("/tmp/rtsp_set.json", JSON_DECODE_ANY, NULL);
	//obj = json_load_file("/tmp/abbe.json", JSON_DECODE_ANY, NULL);
	//path_query_obj bitrate_obj = pq_new();
	pq_set(obj, "bitrate_err", "1");


	//pq_get(obj, "bitrate_err", bitrate_flag, sizeof(bitrate_flag), NULL);
	pq_dump_file(obj, "/tmp/abbe.json");
	//pq_dump_file(obj, "/tmp/rtsp_set.json");
	string=pq_dumps(obj);
	printf("~~~~~~~~~~~~bitrte_flag =%s~~~~~~~~~~~~\n",string);
	pq_free(obj);
#endif
}
#endif


#ifndef _INFILE_DEBUG_H
#define _INFILE_DEBUG_H

#ifdef _cplusplus
exetern "C" {
#endif


//---------------------------------------------
//-- In-File Debug
//--	You should define the following macros in each file which includes this header file.
//--		#define LOG_INFO	0
//--		#define LOG_DBG		0
//--		#define LOG_VERBOSE	0
//--		#define LOG_FUNC	0
//---------------------------------------------

#define NULL_FUNCTION				do {} while(0)
#define error_printf(format, ...)   fprintf(stderr, "\033[0;31m[" "%s() @ %s, %d, ERROR: " format "\033[0m\n", __FUNCTION__, __FILE__, __LINE__,  ##__VA_ARGS__);

#if LOG_INFO==1
#define info_printf(format, ...)		fprintf(stderr, format, ##__VA_ARGS__)
#else
#define info_printf(format, ...)		NULL_FUNCTION
#endif

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

#if LOG_FUNC==1
#define FUNC_ENTRY(format, ...)     fprintf(stderr, "Entering --> %s("format")\n", __FUNCTION__, ##__VA_ARGS__)
#define FUNC_EXIT()                 fprintf(stderr, "Exiting <-- %s()", __FUNCTION__)
#else
#define FUNC_ENTRY(format, ...)     NULL_FUNCTION
#define FUNC_EXIT()                 NULL_FUNCTION
#endif



#ifdef _cplusplus
}
#endif

#endif	// _INFILE_DEBUG_H

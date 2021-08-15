#ifndef _JSON_COMMON_H_
#define _JSON_COMMON_H_

#include <stdio.h>
#include <string.h>
#include <jansson.h>


#ifdef __cplusplus
extern "C" {
#endif


int getJsonString(char *buf, char *key, char **value, int *length);


#ifdef __cplusplus
}
#endif

#endif  // _JSON_COMMON_H_
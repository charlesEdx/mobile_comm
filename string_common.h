#ifndef _STRING_COMMON_H_
#define _STRING_COMMON_H_

#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif


#define STR_EQUAL(s1, s2)           (!strcmp(s1, s2))
#define STR_N_EQUAL(s, s2, n)       (!strncmp(s1, s2, n))

#define STR_COPY(d, s)              strcpy(d, s)
#define STR_N_COPY(d, s, n)         strncpy(d, s, n)

#define SNPRINTF(sz , format, ...)  snprintf(sz, sizeof(sz), format, ##__VA_ARGS__)



#ifdef __cplusplus
}
#endif

#endif  // _STRING_COMMON_H_
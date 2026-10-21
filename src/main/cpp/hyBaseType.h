#if !defined(INC_HYBASE_TYPE_H)
#define INC_HYBASE_TYPE_H

//#include  <unistd.h>
//#include  <stdint.h>
#include  <stdlib.h>
#include  <stdio.h>
//#include  <semaphore.h>
//#include  <errno.h>
#include  <sys/types.h>
//#include  <sys/wait.h>
//#include  <pthread.h>
#include  <string.h>
//#include  <sys/prctl.h>
//#include  <sys/time.h>
//#include  <sys/mount.h>
#include  <time.h>

#ifndef INT64_C
	#define INT64_C(c)	(c ## LL)
	#define UINT64_C(c) (c ## ULL)
#endif


typedef int						HINT32;
typedef unsigned int			HUINT32;
typedef signed short			HINT16;
typedef unsigned short			HUINT16;
typedef			char			HCHAR;
typedef unsigned char			HUCHAR;
typedef signed char				HSCHAR;
//typedef uint64_t				HUINT64;
//typedef int64_t					HINT64;
typedef float					HFLOAT;
typedef double					HDOUBLE;
typedef unsigned char			HBOOL;
typedef void					HVOID;
typedef unsigned long			HULONG;
typedef long					HLONG;
typedef HVOID*					HHANDLE;
#define HTRUE					(HBOOL)1
#define HFALSE					(HBOOL)0


#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
                ((HUINT32)(HUCHAR)(ch0) | ((HUINT32)(HUCHAR)(ch1) << 8) |   \
                ((HUINT32)(HUCHAR)(ch2) << 16) | ((HUINT32)(HUCHAR)(ch3) << 24 ))

#define AV_FRAME_HEADER		MAKEFOURCC('H','Y','A','V')

#endif  



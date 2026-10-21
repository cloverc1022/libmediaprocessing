
#ifndef __MP4_READ_H__
#define __MP4_READ_H__

#ifdef LINUX_ANDROID
		#define INT64_C(c)	(c ## LL)
		#define UINT64_C(c) (c ## ULL)
#else
	#ifndef INT64_C
		#define INT64_C(c)	(c ## LL)
		#define UINT64_C(c) (c ## ULL)
	#endif
#endif

#include "Common.h"

#ifdef __cplusplus
extern "C"{
#endif
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libavutil/avutil.h>
	//#include <libavfilter/avfilter.h> //for ffmpeg 2.3.3
	//#include <libswscale/swscale.h> //for ffmpeg 2.3.3
#ifdef __cplusplus
}
#endif //__cplusplus



class MP4Read
{
public:
	//typedef INT32 (* POnStreamCallback)(UCHAR* pData, UINT32 nDataSize, void* pUserData); //Import: POnStreamCallback defined in mp4.h
	
	MP4Read();
	~MP4Read();

	INT32 OpenMp4File(char *pFilename, void *pArgFileInfo);
	INT32 CloseMp4File();
	INT32 SetBeginPos(INT32 nBeginPos_sec);
	INT32 ReadOneFrame(UCHAR* pBuf, UINT32 nBufMaxSize);

private:
	char m_bOpened;
	char m_chFilename[256];
	INT32  m_nVideoIndex, m_nAudioIndex;
	INT32  m_nBeginPos_sec;

	AVFormatContext *m_pInputFormatContext;
	AVPacket* m_pPacket;
};


#endif
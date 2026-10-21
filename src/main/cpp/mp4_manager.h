
#ifndef __MP4_MANAGER_H__
#define __MP4_MANAGER_H__

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
#include "audio_aac.h"

#ifdef __cplusplus
extern "C"{
#endif
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libavutil/avutil.h>
	//#include <libavfilter/avfilter.h> //for ffmpeg_2.3.3
	//#include <libswscale/swscale.h> //for ffmpeg_2.3.3

	int decG711toPCM(UCHAR *pG711, int nSizG711, UCHAR *pOut); //defined mp4.cpp

#ifdef __cplusplus
}
#endif //__cplusplus



class MP4
{
public:
	MP4();
	~MP4();
	INT32   m_nFreqIdx; //for aac without 7 bytes head

	static UINT32 SEMP4_GetSdkVer(char *chDesc, INT32 nDescMaxSize);

	//Desc:
	//	Get width and height of the video from one i frame
	//Parameter:
	//	in_pData:  the data of the one i frame
	//	nDataSize: the size of the data saved in the in_pData
	//	in_out_array: the two-dimensional array.e.g.:int  arrWH[2]={0,0}; arrWH[0]-->width, arrWH[1]-->height
	//	in_nArraySize:the dimensions of the array.e.g.:2
	//return:
	//	>=0 success
	//	<0  fail
	static int PreGetWHFromIFrame(unsigned char* in_pData, int nDataSize, int* in_out_array, int in_nArraySize);
	static int GetWHDecodeIFrame(unsigned char* in_pData, int nDataSize, int* in_out_array, int in_nArraySize);
	
	int OpenMp4File(char *pFilename, int nVideoWidth, int nVideoHeigh, int nVideoType, INT32 nAudioSampleRate, INT32 nAudioChannel, INT32 nAudioType, INT32 nRotateVideo); //h264 or mjpeg video
	int CloseMp4File();
	int AddVideoFrame(UCHAR *pFrameBuffer,INT32 iSrcPos, int nFrameLen, UINT32 nTimstamp, int bKeyFrame);
	int AddAudioFrame(UCHAR *pFrameBuffer,INT32 iSrcPos, int nFrameLen, UINT32 nTimstamp);
	int AddAudioFrame_g711(UCHAR *pFrameBuffer,INT32 iSrcPos,int nFrameLen, UINT32 nTimstamp);
	int FlushG711Remainder();

	int inner_encG711AtoAAC(UCHAR *pDataG711, int nBytesG711, UCHAR *pBufG711, int *pnSizBufG711,UCHAR *pBufPCM, UCHAR *pBufAAC);

	int AddVideoFrame_ts(UCHAR *pArgFrameBuffer,INT32 iSrcPos, INT32 nFrameLen, UINT32 nTimstamp, INT32 bKeyFrame);
	int AddAudioFrame_ts(UCHAR *pFrameBuffer,INT32 iSrcPos,int nFrameLen, UINT32 nTimstamp);
private:
	AVFormatContext	*m_oc;
	AVStream		*m_pffVStream;
	AVStream		*m_pffAStream;
	UINT32			m_VStartTime;
	UINT32			m_AStartTime;
	UINT32			m_nVideoCodecID;
	INT32			m_nSpsPpsLen; //the size of sps and pps for H264 data, 20150601
	UINT32			m_nVideoFrmNum;
	int64_t			m_nLastVideoPts;

	UINT32	m_ARecLastTimeMs;
	INT32	m_nASamplerate;
	
	INT32   m_nAudioType_origin;
	INT32   m_nSizBufG711;
	UCHAR  *m_pBufG711;
	UCHAR  *m_pBufPCM;
	UCHAR  *m_pBufAAC;

	UINT32  m_nTimstampG711;

	AAC_ENC_CONTEXT *m_pEncCtx;

	pub_mutex_t m_mutexWrite;
	INT32   m_bOpened;
};



class RTSP
{
public:
	typedef INT32 (* RTSP_POnStreamCallback)(UCHAR* pData, UINT32 nDataSize, void* pUserData); //Import: POnStreamCallback defined in mp4.h

	RTSP(RTSP_POnStreamCallback pCallback, void *pUserData);
	~RTSP();

	static UINT32 RTSP_GetSdkVer(char *chDesc, INT32 nDescMaxSize);
	INT32 RTSP_OpenUrl(char *pchUrl, char *pchKeyValue);
	INT32 RTSP_CloseUrl();
	INT32 RTSP_ResetUrl(char *pchUrl);
	void  RTSP_Play();
	void  RTSP_Pause();

protected:
	//--{{for Linux
	pub_threadid m_threadIDRead;
	//--}}for Linux

	//--{{for Windows
	HANDLE m_handleReadThread;
	//--}}for Windows

	int GetIndexBy(int nSampleRate);
	static handle_thread WINAPI ThreadReadStrem(void* parameter);
	void DoReadStream();

	static int AvFormatOpenCallback(void* param);
	int IsReconnect();
	inline int GetStreamNum(){
		int nCount = 0;
		if(m_pInputFormatContext) nCount = m_pInputFormatContext->nb_streams;
		return nCount;
	};

private:
	RTSP_POnStreamCallback m_pCallBack;
	void* m_pUserData;

	CHAR   m_bRunning;
	UINT32 m_dwOpenTime;
	AVFormatContext	*m_pInputFormatContext;
	void*  m_pStreamCallback;
	INT32  m_nVideoIndex, m_nAudioIndex, m_nDataIndex;
	CHAR  *m_pchUrl;

	UCHAR  *m_pBuf;
	pub_mutex_t m_mutexWrite;
};


#endif
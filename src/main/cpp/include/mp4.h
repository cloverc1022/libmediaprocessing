#ifndef __MP4_H__
#define __MP4_H__

#ifdef WIN32DLL
	#define SEMP4_CALLBACK	__stdcall

	#ifdef SE_MP4_EXPORTS
	#define SEMP4_API_API  __declspec(dllexport)
	#else
	#define SEMP4_API_API __declspec(dllimport)
	#endif
#else
	#define SEMP4_EXTERN_C
	#define SEMP4_API_API 
	#define SEMP4_CALLBACK
#endif //// #ifdef WIN32DLL

#include "SEP2P_Type.h"


typedef enum
{
	RTSP_AV_CODECID_UNKNOWN		= 0x00,
	RTSP_AV_CODECID_VIDEO_MJPEG,
	RTSP_AV_CODECID_VIDEO_H264,
	RTSP_AV_CODECID_VIDEO_H265,

	RTSP_AV_CODECID_AUDIO_ADPCM	=0x100,
	RTSP_AV_CODECID_AUDIO_G726,
	RTSP_AV_CODECID_AUDIO_G711A,
	RTSP_AV_CODECID_AUDIO_AAC,

	RTSP_AV_CODECID_DATA_ALARM =0x200
}RTSP_ENUM_AV_CODECID;
typedef enum
{
	RTSP_VIDEO_FRAME_FLAG_I	= 0x00,	// Video I Frame
	RTSP_VIDEO_FRAME_FLAG_P	= 0x01,	// Video P Frame
	RTSP_VIDEO_FRAME_FLAG_B	= 0x02,	// Video B Frame
}RTSP_ENUM_VIDEO_FRAME;


typedef enum
{
	MP4_AUDIO_SAMPLE_8K		= 0x00,
	MP4_AUDIO_SAMPLE_11K	= 0x01,
	MP4_AUDIO_SAMPLE_12K	= 0x02,
	MP4_AUDIO_SAMPLE_16K	= 0x03,
	MP4_AUDIO_SAMPLE_22K	= 0x04,
	MP4_AUDIO_SAMPLE_24K	= 0x05,
	MP4_AUDIO_SAMPLE_32K	= 0x06,
	MP4_AUDIO_SAMPLE_44K	= 0x07,
	MP4_AUDIO_SAMPLE_48K	= 0x08,
}MP4_ENUM_AUDIO_SAMPLERATE;
typedef enum
{
	MP4_AUDIO_DATABITS_8	= 0,
	MP4_AUDIO_DATABITS_16	= 1,
}MP4_ENUM_AUDIO_DATABITS;
typedef enum
{
	MP4_AUDIO_CHANNEL_MONO	= 0,
	MP4_AUDIO_CHANNEL_STERO	= 1,
}MP4_ENUM_AUDIO_CHANNEL;

typedef struct tag_FILE_INFO
{
	RTSP_ENUM_AV_CODECID eVideoAVCodecID;
	INT32   nVideoWidth;
	INT32   nVideoHeight;
	
	RTSP_ENUM_AV_CODECID eAudioAVCodecID;
	INT32   nAudioSamplerate;
	INT32   nAudioDatabit;
	INT32   nAudioChannels;

	INT32   nDurationInSecond;
	INT64   nFileSizeInByte;
	CHAR    reserve[64];
}FILE_INFO;

typedef struct rtsp_stStreamHead
{
	UINT32 nAVCodecID;		 // refer to RTSP_ENUM_AV_CODECID
	//nParameter:
	//		Video: refer to RTSP_ENUM_VIDEO_FRAME. 
	//		Audio:(samplerate << 2) | (databits << 1) | (channel), samplerate refer to MP4_ENUM_AUDIO_SAMPLERATE; databits refer to MP4_ENUM_AUDIO_DATABITS; channel refer to MP4_ENUM_AUDIO_CHANNEL
	CHAR   nParameter;	 
	CHAR   reserve1[3];

	UINT32 nStreamDataLen;	// Stream data size after following struct 'RTSP_STREAM_HEAD'
	UINT32 nTimestamp;		// Timestamp of the frame, in milliseconds
	CHAR   reserve2[8];
}RTSP_STREAM_HEAD;


#ifdef TEST_FFMPEG_31
	extern const uint8_t ff_reverse[256];
	extern const uint8_t ff_log2_tab[256];
#endif

typedef INT32 (* RTSP_POnStreamCallback)(UCHAR* pData, UINT32 nDataSize, void* pUserData);

#ifdef __cplusplus
extern "C"
{
#endif

	

	SEMP4_API_API UINT32 SEMP4_GetSdkVer(CHAR *chDesc, INT32 nDescMaxSize);
	SEMP4_API_API INT32  SEMP4_PreGetWHFromIFrame(UCHAR* in_pData, INT32 nDataSize, INT32* in_out_array, INT32 in_nArraySize);
	SEMP4_API_API UINT32 SEMP4_Init(CHAR *pStrAndroidPackageName, INT32 nAndroidBuildVersion);
	SEMP4_API_API void   SEMP4_Uninit();

		
	//----{{Write MP4 API----	
		SEMP4_API_API UINT32 SEMP4_Create(UCHAR **ppHandle);
		SEMP4_API_API void   SEMP4_Destroy(UCHAR **ppHandle);

		//parameter:(for mono)
		//	nSampleRate is among of g_nSampleRate
		//		static INT32 g_nSampleRate[] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350};
		//	pBufOut: the buffer outputing. its size is at least (7+nSizeOfES);
		//
		SEMP4_API_API INT32 SEMP4_AppendAACHead(UCHAR *pHandle,CHAR *pBufOut, INT32 nSampleRate, CHAR *pAudioData,INT32 iSrcPos, INT32 nSizeOfES);

		//param:
		//		nVideoType: refer to RTSP_ENUM_AV_CODECID
		//		nAudioType: refer to RTSP_ENUM_AV_CODECID
		//		nRotateVideo: rotation degree for video, its rang is (0,360)
		//return:
		//	OK:  >=0
		//	fail: <0
		SEMP4_API_API INT32  SEMP4_OpenMp4File(UCHAR *pHandle, CHAR *pFilename, INT32 nVideoWidth, INT32 nVideoHeigh, INT32 nVideoType, 
											   INT32 nAudioSampleRate, INT32 nAudioChannel, INT32 nAudioType, INT32 nRotateVideo); //h264 or mjpeg video
		SEMP4_API_API INT32  SEMP4_CloseMp4File(UCHAR *pHandle);
		//the first frame must be I frame for H264 video
		SEMP4_API_API INT32  SEMP4_AddVideoFrame(UCHAR *pHandle, UCHAR *pFrameBuffer,INT32 iSrcPos, INT32 nFrameLen, UINT32 nTimstamp, INT32 bKeyFrame);
		//return:
		//		-2: please first call SEMP4_OpenMp4File
		//		-3: audio timestamp < video timestamp
		//		-4: first frame must be video data
		//		-5: there is not audio track
		SEMP4_API_API INT32  SEMP4_AddAudioFrame(UCHAR *pHandle, UCHAR *pFrameBuffer,INT32 iSrcPos, INT32 nFrameLen, UINT32 nTimstamp);
	//----}}Write MP4 API----

		//desc:
		//	this API is independent.It has nothing to do with other APIs.
		//Parameter:
		//	pTsFilelistBytime: ts filename list inputed,they are sorted in ascending time. multiple files are divided by ','. e.g.: /aa/1.ts,/aa/2.ts
		//	eVideoType, eAudioType: refer to RTSP_ENUM_AV_CODECID
		//			if it is RTSP_AV_CODECID_UNKNOWN this API will auto detect audio/video encoding type from the first ts file
		//			if eAudioType's value is -1 then there is NOT audio. 
		//	nAudioSampleRate: audio sample rate. 8K is 8000, 16K is 16000
		//	nAudioChannel: its value is 1 or 2
		//	pOutFilename: mp4 filename outputed
		//	nRotateVideo: rotation degree for video. its rang is [0,360). 0 means no rotation.
		//
		//return:
		//	0: ok
		//		-1: pTsFilelistBytime=NULL or pOutFilename=NULL
		//		-2: failed to create mp4 context
		//		-3: SEMP4_OpenMp4File(.)<0
		//		-4: failed to read ts file
		//		-5: failed to create decoding ts stream context
		//		-6: unknown video encoding format
		//
		SEMP4_API_API INT32  TS2MP4(CHAR *pTsFilelistBytime,INT32 eVideoType, INT32 eAudioType,INT32 nAudioSampleRate,INT32 nAudioChannel, CHAR *pOutFilename, INT32 nRotateVideo);


	//----{{Read MP4 API----
		SEMP4_API_API UINT32 SEMP4Read_Create(UCHAR **ppHandle);
		SEMP4_API_API void   SEMP4Read_Destroy(UCHAR **ppHandle);
		//return:
		//	>=0 OK
		//	 <0 FAIL
		//		-1: pFilename==NULL
		//		-2: open file fails
		//		-3: there isn't audio and video track
		SEMP4_API_API INT32  SEMP4Read_OpenMp4File(UCHAR *pHandle, CHAR *pFilename, FILE_INFO *pFileInfo);
		SEMP4_API_API INT32  SEMP4Read_CloseMp4File(UCHAR *pHandle);

		//Parameter:
		//  nBeginPos_sec, 
		//		if it is >0 then one frame is getted to start the specified position in second.
		//		if it is <=0 then one frame is getted one by one.
		SEMP4_API_API INT32  SEMP4Read_SetBeginPos(UCHAR *pHandle, INT32 nBeginPos_sec);

		//Parameter:
		//	pBuf=RTSP_STREAM_HEAD + raw_av_data
		//
		//return:
		//	>=0 OK
		//	< 0 FAIL
		//		-1: nBufMaxSize<384KBytes
		//		-2: please first call SEMP4Read_OpenMp4File
		//		-3: read one frame fails
		SEMP4_API_API INT32  SEMP4Read_ReadOneFrame(UCHAR *pHandle,UCHAR* pBuf, UINT32 nBufMaxSize);
	//----}}Read MP4 API----



	//----{{RTSP API----
		//OK:  >=0
		//fail: <0
		SEMP4_API_API UINT32 RTSP_GetSdkVer(CHAR *chDesc, INT32 nDescMaxSize);
		SEMP4_API_API UINT32 RTSP_Create(UCHAR **ppHandle, RTSP_POnStreamCallback pOnStreamCallback, void *pUserData);
		SEMP4_API_API void   RTSP_Destroy(UCHAR **ppHandle);
		SEMP4_API_API INT32  RTSP_OpenUrl(UCHAR *pHandle, CHAR *pchUrl, CHAR *pchKeyValue);
		SEMP4_API_API INT32  RTSP_ResetUrl(UCHAR *pHandle, CHAR *pchUrl);
		SEMP4_API_API INT32  RTSP_CloseUrl(UCHAR *pHandle);
	//----}}RTSP API----
#ifdef __cplusplus
}
#endif

#endif
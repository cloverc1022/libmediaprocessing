
#include "mp4_read.h"
#include "mp4.h"

#if defined(LINUX_ANDROID) && !defined(BUILD_FOR_OHOS)
	#include <dlfcn.h>
	#include "SEMP4_android.h"
#endif

//----{{IsLegalAPIforMP4-----------------------

// 
// #ifdef WIN32DLL
// 	#pragma comment(lib,"libgcc.a")
// 	#pragma comment(lib,"libmingwex.a")
// 	#pragma comment(lib,"ws2_32.lib")
// 	#pragma comment(lib,"Advapi32.lib")
// 	#pragma comment(lib,"libavcodec.a")
// 	#pragma comment(lib,"libavformat.a")
// 	#pragma comment(lib,"libavutil.a")
// 	#pragma comment(lib,"libswresample.a")
// #endif

#define	TIME_BASE_AV		90000
#define	TIME_ST_FREQ		90

#if defined(LINUX_ANDROID) && !defined(BUILD_FOR_OHOS)
	#include "SEMP4_android.h"
	//#define TRACE	LOGI		//juju
	#define TRACE(...)

	extern  char IsLegalAPIforMP4();
#else
	//#define TRACE printf
	#define TRACE(...)
#endif


#define MAX_SIZE_PLAYBACK_BUF		384*1024
#define ADTS_HEADER_LEN				7
static char  NAL_START[]				={ 0,0,0,1 };
static UCHAR ADTSHeader[ADTS_HEADER_LEN]={ 0xFF, 0xF9, 0x60, 0x40, 0x00, 0x1F, 0xFC};

static char *pub_strncpy_safe(char *strDest, size_t destBufMaxSize, const char *strSource, size_t src_count)
{
	size_t nCount=(destBufMaxSize>src_count) ? src_count : destBufMaxSize;
	char* pDest=strncpy(strDest, strSource, nCount);

	if(nCount<destBufMaxSize) *(pDest+nCount)='\0';
	else *(pDest+nCount-1)='\0';

	return pDest;
}
static void pub_Sleep(ULONG ms)
{
#ifndef WIN32
	usleep(ms * 1000);
#else
	Sleep(ms);
#endif
}
//-----------------------------------------------------------------------------------------------------------

MP4Read::MP4Read()
{
	av_register_all();

	m_bOpened=0;
	memset(m_chFilename, 0, sizeof(m_chFilename));
	m_nVideoIndex=-1;
	m_nAudioIndex=-1;
	m_nBeginPos_sec=-1;
	m_pInputFormatContext=NULL;
	m_pPacket=NULL;
}

MP4Read::~MP4Read()
{
	if(m_pPacket){
		av_free_packet(m_pPacket);
		m_pPacket=NULL;
	}
}

INT32 MP4Read::OpenMp4File(char *pFilename,void *pArgFileInfo)
{
	if(pFilename==NULL) return -1;
	AVDictionary *pAVOptions = 0;
	INT32 nRet=0, i=0;
    FILE_INFO *pFileInfo=(FILE_INFO *)pArgFileInfo;
	CloseMp4File();

	pub_strncpy_safe(m_chFilename, sizeof(m_chFilename), pFilename, strlen(pFilename));
	
	av_dict_set(&pAVOptions, "stimeout", "200000", 0);
	//av_dict_set(&pAVOptions, "initial_pause", "1", 0); //invalid for mp4 file
	nRet = avformat_open_input(&m_pInputFormatContext, m_chFilename, NULL, &pAVOptions);
	if(pAVOptions!=NULL) {
		av_dict_free(&pAVOptions);
		pAVOptions=NULL;
	}
	if(nRet!=0) {
		//TRACE_DEBUG(0x1<<0xE,"doPlayback] avformat_open_input fail\n");
		nRet=-2;
		goto FUN_OVER;
	}

	m_nVideoIndex=-1; //20161118
	m_nAudioIndex=-1; //20161118
	for(i=0; i<m_pInputFormatContext->nb_streams; i++){
		if(m_pInputFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) m_nVideoIndex = i;
		else if(m_pInputFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) m_nAudioIndex = i;
	}
	if(m_nVideoIndex==-1 && m_nAudioIndex==-1){
		nRet=-3;
		goto FUN_OVER;
	}

	
	if(pFileInfo){
		//video
		if(m_nVideoIndex<0){
			pFileInfo->eVideoAVCodecID=RTSP_AV_CODECID_UNKNOWN;
			pFileInfo->nVideoWidth =0;
			pFileInfo->nVideoHeight=0;
		}else{
			switch(m_pInputFormatContext->streams[m_nVideoIndex]->codec->codec_id)
			{
				case AV_CODEC_ID_MJPEG:
					pFileInfo->eVideoAVCodecID=RTSP_AV_CODECID_VIDEO_MJPEG;
					break;

				case AV_CODEC_ID_H264:
					pFileInfo->eVideoAVCodecID=RTSP_AV_CODECID_VIDEO_H264;
					break;

				default:
					pFileInfo->eVideoAVCodecID=RTSP_AV_CODECID_UNKNOWN;
			}
			pFileInfo->nVideoWidth =m_pInputFormatContext->streams[m_nVideoIndex]->codec->width;
			pFileInfo->nVideoHeight=m_pInputFormatContext->streams[m_nVideoIndex]->codec->height;
		}
		//audio
		if(m_nAudioIndex<0){
			pFileInfo->eAudioAVCodecID=RTSP_AV_CODECID_UNKNOWN;
			pFileInfo->nAudioSamplerate=0;
			pFileInfo->nAudioDatabit=0;
			pFileInfo->nAudioChannels=0;
		}else{
			switch(m_pInputFormatContext->streams[m_nAudioIndex]->codec->codec_id)
			{
				case AV_CODEC_ID_ADPCM_G726:
					pFileInfo->eAudioAVCodecID=RTSP_AV_CODECID_AUDIO_G726;
					break;

				case AV_CODEC_ID_PCM_ALAW:
					pFileInfo->eAudioAVCodecID=RTSP_AV_CODECID_AUDIO_G711A;
					break;

				case AV_CODEC_ID_AAC:
					pFileInfo->eAudioAVCodecID=RTSP_AV_CODECID_AUDIO_AAC;
					break;
				default:
					pFileInfo->eAudioAVCodecID=RTSP_AV_CODECID_UNKNOWN;
			}
			pFileInfo->nAudioSamplerate=m_pInputFormatContext->streams[m_nAudioIndex]->codec->sample_rate;
			pFileInfo->nAudioDatabit=16;
			pFileInfo->nAudioChannels=m_pInputFormatContext->streams[m_nAudioIndex]->codec->channels;
		}
		pFileInfo->nDurationInSecond=INT32(m_pInputFormatContext->duration/1000000); //in second;
		pFileInfo->nFileSizeInByte=avio_size(m_pInputFormatContext->pb);//in byte;
	}

	//
	m_nBeginPos_sec=-1;
	m_bOpened=1;
	if(m_pPacket==NULL){
		m_pPacket = new AVPacket;
		av_init_packet(m_pPacket);
	}
FUN_OVER:
	return nRet;
}

INT32 MP4Read::CloseMp4File()
{
	INT32 nRet=0;
	m_bOpened=0;
	if(m_pInputFormatContext){
		avformat_close_input(&m_pInputFormatContext);
		m_pInputFormatContext = 0;
		//TRACE_DEBUG(0x1<<0xE,"CP2PConnect::doPlayback, avformat_close_input(..)\n");
	}
	return nRet;
}

INT32 MP4Read::SetBeginPos(INT32 nBeginPos_sec)
{
	if(m_bOpened==0) return -2;
	m_nBeginPos_sec=nBeginPos_sec;
    return 0;
}

//return:
//	>=0 ok
//	 <0 fail
//		-1: nBufMaxSize<MAX_SIZE_PLAYBACK_BUF
//		-2: m_bOpened==0
//		-3: av_read_frame(.)<0
//		-4: nSPSLen<=0
INT32 MP4Read::ReadOneFrame(UCHAR* pBuf, UINT32 nBufMaxSize)
{
	if(nBufMaxSize<MAX_SIZE_PLAYBACK_BUF) return -1;
	if(m_bOpened==0) return -2;
	
	static AVRational stTimeBaseUs = {1, 1000};

	INT32 nRet=0, nReadRet=0;
	CHAR  chSPSPPS[80]={0};
	INT32 nPTS=0, nAudioFrameSize=0;
	UINT32 nSleepUS=60000;
	INT32 nSPSLen=0, nPPSLen=0;
	AVCodecContext  *pAVCodecC=NULL;
	
	RTSP_STREAM_HEAD *pHead=(RTSP_STREAM_HEAD *)pBuf;
	UCHAR *pRawData=pBuf+sizeof(RTSP_STREAM_HEAD);
	memset(pBuf, 0, nBufMaxSize);

	if(m_nBeginPos_sec>0){
		nReadRet=av_seek_frame(m_pInputFormatContext, -1, m_nBeginPos_sec*AV_TIME_BASE, AVSEEK_FLAG_BACKWARD);
		m_nBeginPos_sec=-1;
	}

	do{
		nReadRet = av_read_frame(m_pInputFormatContext, m_pPacket);
		if(nReadRet>=0){
			if(m_pPacket->stream_index == m_nVideoIndex){
				pAVCodecC=m_pInputFormatContext->streams[m_nVideoIndex]->codec;

				nPTS = (INT32)av_rescale_q(m_pPacket->pts, m_pInputFormatContext->streams[m_nVideoIndex]->time_base, stTimeBaseUs);
				if((m_pPacket->flags&AV_PKT_FLAG_KEY)== AV_PKT_FLAG_KEY)
				{
					switch(pAVCodecC->codec_id){
						case AV_CODEC_ID_MJPEG:{
							pHead->nAVCodecID=RTSP_AV_CODECID_VIDEO_MJPEG;
							if(m_pPacket->size>0) memcpy(pRawData, m_pPacket->data, m_pPacket->size);
							}break;

						case AV_CODEC_ID_H264:{
							pHead->nAVCodecID=RTSP_AV_CODECID_VIDEO_H264;
							//I frame
							//if(pAVCodecC->width>=1920) nSleepUS=30000;
							//else nSleepUS=10000;

							//extradata[0]   version
							//extradata[1]   AVCProfileIndication
							//extradata[2]   profile_compatibility
							//extradata[3]   AVCLevelIndication
							//extradata[4]   NALU length
							//extradata[5]   sps num, low 5 bit
							//extradata[6,7] sps_length

							//extradata[8]   sps[0]
							//......

							//extradata[8+sps_length]                   pps num
							//extradata[8+sps_length+1, 8+sps_length+2] pps_length

							//pps content
							//
							memcpy(chSPSPPS, NAL_START,4);
							nSPSLen=pAVCodecC->extradata[6]<<8 | pAVCodecC->extradata[7];
							if(nSPSLen<=0) {
								nRet=-4;
								goto FUN_OVER;
							}
							memcpy(chSPSPPS+4, &pAVCodecC->extradata[8], nSPSLen);
							memcpy(chSPSPPS+4+nSPSLen, NAL_START,4);
							nPPSLen=pAVCodecC->extradata[8+nSPSLen+1]<<8 | pAVCodecC->extradata[8+nSPSLen+2];
							memcpy(chSPSPPS+4+nSPSLen+4, &pAVCodecC->extradata[8+nSPSLen+2+1], nPPSLen);

							nSPSLen=4+nSPSLen+4+nPPSLen;

							//begin to send...
							memcpy(pRawData, chSPSPPS, nSPSLen);
							if(m_pPacket->size>0){
								m_pPacket->data[0]=0; 
								m_pPacket->data[1]=0;
								m_pPacket->data[2]=0;
								m_pPacket->data[3]=1;
								memcpy(pRawData+nSPSLen, m_pPacket->data, m_pPacket->size);
							}
							}
							break;

						default:{
							pHead->nAVCodecID=RTSP_AV_CODECID_UNKNOWN;
							memcpy(pRawData, m_pPacket->data, m_pPacket->size);
							}
					}
					pHead->nParameter	 =RTSP_VIDEO_FRAME_FLAG_I;
					pHead->nStreamDataLen=nSPSLen+m_pPacket->size;
					pHead->nTimestamp    =nPTS;

				}else{
					switch(pAVCodecC->codec_id){
						case AV_CODEC_ID_MJPEG:{
							pHead->nAVCodecID=RTSP_AV_CODECID_VIDEO_MJPEG;
							if(m_pPacket->size>0) memcpy(pRawData, m_pPacket->data, m_pPacket->size);
							}break;

						case AV_CODEC_ID_H264:{
							pHead->nAVCodecID=RTSP_AV_CODECID_VIDEO_H264;
							if(m_pPacket->size>0){
								//P frame
								//begin to send...
								m_pPacket->data[0]=0; 
								m_pPacket->data[1]=0;
								m_pPacket->data[2]=0;
								m_pPacket->data[3]=1;
								memcpy(pRawData, m_pPacket->data, m_pPacket->size);
							}
							}break;

						default:{
							pHead->nAVCodecID=RTSP_AV_CODECID_UNKNOWN;
							if(m_pPacket->size>0) memcpy(pRawData, m_pPacket->data, m_pPacket->size);
							}
					}
					pHead->nParameter	 =RTSP_VIDEO_FRAME_FLAG_P;
					pHead->nStreamDataLen=m_pPacket->size;
					pHead->nTimestamp    =nPTS;
					//nReadRet=sendPlaybackData(pObjCam, AV_V_FRAME_P, pTmpBuf, pPacket->size, nPTS, nPlaybackID);
					
				}
				//pub_Sleep(nSleepUS);
			}else if(m_pPacket->stream_index == m_nAudioIndex){
				nPTS = (INT32)av_rescale_q(m_pPacket->pts, m_pInputFormatContext->streams[m_nAudioIndex]->time_base, stTimeBaseUs);
				pAVCodecC=m_pInputFormatContext->streams[m_nAudioIndex]->codec;
				if(pAVCodecC->codec_id==AV_CODEC_ID_AAC){
					nAudioFrameSize= ADTS_HEADER_LEN+m_pPacket->size;
					ADTSHeader[3] = 0x40 | ((nAudioFrameSize & 0xFFFF) >> 11);
					ADTSHeader[4] = (nAudioFrameSize & 0x7FF) >> 3;
					ADTSHeader[5] = 0x1F | ((nAudioFrameSize & 0x0007) << 5);

					//begin to send...
					memcpy(pRawData, ADTSHeader, ADTS_HEADER_LEN);
					memcpy(pRawData+ADTS_HEADER_LEN, m_pPacket->data, m_pPacket->size);

					pHead->nAVCodecID=RTSP_AV_CODECID_AUDIO_AAC;
					pHead->nParameter	 =0x0E; //16K,16bit,mono
					pHead->nStreamDataLen=ADTS_HEADER_LEN+m_pPacket->size;

				}else{
					switch(pAVCodecC->codec_id){
						case AV_CODEC_ID_PCM_ALAW:
							pHead->nAVCodecID=RTSP_AV_CODECID_AUDIO_G711A;
							break;

						case AV_CODEC_ID_ADPCM_G726:
							pHead->nAVCodecID=RTSP_AV_CODECID_AUDIO_G726;
							break;

						case AV_CODEC_ID_ADPCM_IMA_QT:
							pHead->nAVCodecID=RTSP_AV_CODECID_AUDIO_ADPCM;
							break;

						default:
							pHead->nAVCodecID=RTSP_AV_CODECID_UNKNOWN;
					}
					memcpy(pRawData, m_pPacket->data, m_pPacket->size);
					pHead->nParameter	 =0x02; //8K,16bit,mono
					pHead->nStreamDataLen=m_pPacket->size;
				}
				pHead->nTimestamp    =nPTS;
				//nReadRet=sendPlaybackData(pObjCam, AV_A_FRAME, pTmpBuf, ADTS_HEADER_LEN+pPacket->size, nPTS, nPlaybackID);
			}
			if(m_pPacket) av_free_packet(m_pPacket);
		}else {
			nRet=-3;
			break;
		}
	}while(0);

FUN_OVER:
	return nRet;
}




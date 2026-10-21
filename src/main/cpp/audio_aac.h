#ifndef INC_AUDIO_AAC_H
#define INC_AUDIO_AAC_H

#ifdef __cplusplus
	extern "C"{

	#include "libavformat/avformat.h"
	#include "libavcodec/avcodec.h"
	#include "libswresample/swresample.h"
	#include "libavutil/time.h"
	#include "libavutil/opt.h"

#endif
	typedef int HINT32;
	typedef unsigned int HUINT32;
	typedef unsigned char  HUCHAR;
	typedef char HCHAR;

	typedef struct {
		AVCodec *m_pEncodeCodec;
		AVCodecContext *m_pEncodeCodecCtx;
		AVFrame  *m_pEncodedFrame;
		AVPacket m_stEncodeAVpkt;
		uint8_t  *m_pEncodeBufResult;
		SwrContext *m_pEncodeSwt;
		uint8_t *m_pEncodeBufSwr;
		int m_nEncodeBufSizeSwr;
		unsigned int m_nSampleRate;
	}AAC_ENC_CONTEXT;

	AAC_ENC_CONTEXT*  aacEnc_init(HINT32 nSampleRate,HINT32 nChn,HINT32 nSampleBits,HINT32 nBitRate);	
	HINT32   aacEnc_exit(AAC_ENC_CONTEXT *pAACEncContext);
	//first use aacEnc_makeHead, then use aacEnc_updateHead
	void  	 aacEnc_makeHead(HCHAR *pBufOut, HINT32 nSampleRate, HINT32 nSizeOfES);
	void  	 aacEnc_updateHead(HCHAR *pBufOut, HINT32 nSizeOfES);
	HINT32   aacEnc_frame_with_adts_head(AAC_ENC_CONTEXT *pAACEncContext,HUCHAR *src, HUINT32 srclen,HUCHAR *dest, HINT32 *dstlen);

	
#ifdef __cplusplus
}
#endif

#endif


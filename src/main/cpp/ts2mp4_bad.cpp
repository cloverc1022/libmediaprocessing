
#include "mp4_manager.h"
#include "mp4_read.h"
#include "mp4.h"

#include "mpeg-ps.h"
#include "mpeg-ts.h"
#include "mpeg-ts-proto.h"
#include <stdio.h>
#include <assert.h>
#include "audio_aac.h"
#include "ts2mp4.h"

#ifdef LINUX_JZ
	#define INT64_MIN	(-0x7fffffffffffffff - 1)
	#define INT64_MAX	0x7fffffffffffffff
#endif



extern INT32 SEMP4_AddAudioFrame_ts(UCHAR *pHandle, UCHAR *pFrameBuffer,INT32 iSrcPos, INT32 nFrameLen, UINT32 nTimstamp);


signed short _A2l[256] = {
	-5504, -5248, -6016, -5760, -4480, -4224, -4992, -4736,
	-7552, -7296, -8064, -7808, -6528, -6272, -7040, -6784,
	-2752, -2624, -3008, -2880, -2240, -2112, -2496, -2368,
	-3776, -3648, -4032, -3904, -3264, -3136, -3520, -3392,
	-22016, -20992, -24064, -23040, -17920, -16896, -19968, -18944,
	-30208, -29184, -32256, -31232, -26112, -25088, -28160, -27136,
	-11008, -10496, -12032, -11520, -8960, -8448, -9984, -9472,
	-15104, -14592, -16128, -15616, -13056, -12544, -14080, -13568,
	-344, -328, -376, -360, -280, -264, -312, -296,
	-472, -456, -504, -488, -408, -392, -440, -424,
	-88, -72, -120, -104, -24, -8, -56, -40,
	-216, -200, -248, -232, -152, -136, -184, -168,
	-1376, -1312, -1504, -1440, -1120, -1056, -1248, -1184,
	-1888, -1824, -2016, -1952, -1632, -1568, -1760, -1696,
	-688, -656, -752, -720, -560, -528, -624, -592,
	-944, -912, -1008, -976, -816, -784, -880, -848,
	5504, 5248, 6016, 5760, 4480, 4224, 4992, 4736,
	7552, 7296, 8064, 7808, 6528, 6272, 7040, 6784,
	2752, 2624, 3008, 2880, 2240, 2112, 2496, 2368,
	3776, 3648, 4032, 3904, 3264, 3136, 3520, 3392,
	22016, 20992, 24064, 23040, 17920, 16896, 19968, 18944,
	30208, 29184, 32256, 31232, 26112, 25088, 28160, 27136,
	11008, 10496, 12032, 11520, 8960, 8448, 9984, 9472,
	15104, 14592, 16128, 15616, 13056, 12544, 14080, 13568,
	344, 328, 376, 360, 280, 264, 312, 296,
	472, 456, 504, 488, 408, 392, 440, 424,
	88, 72, 120, 104, 24, 8, 56, 40,
	216, 200, 248, 232, 152, 136, 184, 168,
	1376, 1312, 1504, 1440, 1120, 1056, 1248, 1184,
	1888, 1824, 2016, 1952, 1632, 1568, 1760, 1696,
	688, 656, 752, 720, 560, 528, 624, 592,
	944, 912, 1008, 976, 816, 784, 880, 848,
	};
static int g711a_Decode( unsigned char *src, unsigned int srclen, unsigned char *dest, unsigned int *dstlen){
	unsigned int i=0;
	unsigned short *pd=(unsigned short*)dest;
	for(i=0; i<srclen; i++) pd[i]=(unsigned short)_A2l[src[i]];
	if(dstlen) *dstlen = srclen<<1;

	return 1;
}
int decG711toPCM(UCHAR *pG711, int nSizG711, UCHAR *pOut){
	return g711a_Decode(pG711, nSizG711, pOut, NULL);
}

int64_t calcuVideoTS(int64_t pts, int64_t *pnLastTSofVideo, int *pnCountVideo, int64_t *pnReferTSVideo, int64_t *pnDiffOfVideo,int64_t *pnCurTsOfVideo, int64_t *pnLastCurTsOfVideo){
	//calcu the 3rd times different pts/90
	int64_t nCurTs=0LL, nTmp=0LL;
	if(*pnLastTSofVideo==0LL) *pnLastTSofVideo=pts/90;
	else{
		if(*pnCountVideo<3){
			*pnDiffOfVideo=pts/90 - *pnLastTSofVideo;
			*pnCountVideo++;
		}		
	}

	nTmp=pts/90 - *pnLastTSofVideo;
	if(nTmp<0){
		*pnReferTSVideo=pts/90;
		nCurTs= *pnLastCurTsOfVideo+*pnDiffOfVideo/2;
		*pnCurTsOfVideo=nCurTs;
	}else{
		if(*pnDiffOfVideo!=0LL && nTmp> 4*(*pnDiffOfVideo)){ //when next frame pts is too bigger
			*pnReferTSVideo=pts/90;
			nCurTs= *pnLastCurTsOfVideo+*pnDiffOfVideo/2;
			*pnCurTsOfVideo=nCurTs;
			/*
			//reset refer to begin point
			nCurTs=*pnLastTSofVideo-*pnReferTSVideo+*pnDiffOfVideo;
			*pnCurTsOfVideo=nCurTs;

			*pnLastTSofVideo=0LL;
			*pnCountVideo=0LL;
			*pnDiffOfVideo=0LL;
			*pnReferTSVideo=pts/90;
			*/
		}else{
			//refer to begin point
			if(*pnReferTSVideo==0LL) {
				*pnReferTSVideo=pts/90;
				nCurTs=0LL;
			}else{
				nCurTs= *pnCurTsOfVideo+pts/90 - *pnReferTSVideo;
			}
		}
	}

	*pnLastTSofVideo=pts/90;
	*pnLastCurTsOfVideo=nCurTs;
	return nCurTs;
}
int64_t calcuAudioTS(int64_t pts, int64_t *pnLastTSofAudio, int *pnCountAudio, int64_t *pnDiffOfAudio, int64_t* pnReferTSAudio, int64_t *pnLastCurTsOfAudio, int64_t *pnCurTsOfAudio)
{
	//calcu the 3rd times different pts/90
	int64_t nCurTs=0LL, nTmp=0LL;
	if(*pnLastTSofAudio==0LL) *pnLastTSofAudio=pts/90;
	else{
		if(*pnCountAudio<5){
			*pnDiffOfAudio = (pts/90 - *pnLastTSofAudio);
			*pnCountAudio++;
		}
	}

	nTmp=pts/90 - *pnLastTSofAudio;
	if(nTmp<0){
		*pnReferTSAudio=pts/90;
		nCurTs= *pnLastCurTsOfAudio+*pnDiffOfAudio/2;
		*pnCurTsOfAudio=nCurTs;
	}else{
		if(*pnDiffOfAudio!=0LL && nTmp> 4*(*pnDiffOfAudio)){ //when next frame pts is too bigger
			*pnReferTSAudio=pts/90;
			nCurTs= *pnLastCurTsOfAudio+*pnDiffOfAudio/2;
			*pnCurTsOfAudio=nCurTs;

			/*
			//reset refer to begin point
			nCurTs=*pnLastTSofAudio-*pnReferTSAudio+*pnDiffOfAudio;
			*pnCurTsOfAudio=nCurTs;

			*pnLastTSofAudio=0LL;
			*pnCountAudio=0LL;
			*pnDiffOfAudio=0LL;
			*pnReferTSAudio=pts/90;
			*/
		}else{
			//refer to begin point
			if(*pnReferTSAudio==0LL) {
				*pnReferTSAudio=pts/90;
				nCurTs=0LL;
			}else{
				nCurTs= *pnCurTsOfAudio+pts/90 - *pnReferTSAudio;
			}
		}
	}
	*pnLastTSofAudio=pts/90;
	*pnLastCurTsOfAudio=nCurTs;
	return (nCurTs);
}


//return:
//	-12: the data is too little.
//	-13: failed to aacEnc_frame_with_adts_head
//
int encG711AtoAAC(char *pDataG711, int nBytesG711, HUCHAR *pBufG711, int *pnSizBufG711,HUCHAR *pBufPCM, HUCHAR *pBufAAC){
	int nSiz=0;

	//1. assemble inBuf to 1K g711A
	if(*pnSizBufG711 < G711_DATA_SIZES) {
		memcpy(pBufG711 + *pnSizBufG711, pDataG711, nBytesG711);
		*pnSizBufG711 += nBytesG711;
		return -12;
	}else{
		int nLenAfterEnc=AAC_ENC_SIZES;
		//把本次数据加上
		memcpy(pBufG711 + *pnSizBufG711, pDataG711, nBytesG711);
		*pnSizBufG711 += nBytesG711;

		//g711 --> pcm, get 2K pcm data
		decG711toPCM(pBufG711, G711_DATA_SIZES, pBufPCM); //pBufG711, pBufPCM: 2K;
		//g711-->pcm的g711数据移除, pBufG711余下的数据移到buf头
		memcpy(pBufG711, pBufG711+G711_DATA_SIZES,  *pnSizBufG711-G711_DATA_SIZES);
		*pnSizBufG711 -= G711_DATA_SIZES; //减去本次解码的g711数据
		
		#if 0
			if(!g_fpLog) {
				static char cFilename[240] = { "d:\\dataTest\\ts.g711.pcm" };
				g_fpLog = fopen(cFilename, "wb");
				}
			if(g_fpLog) {
				fwrite(pBufPCM, 1, G711_DATA_SIZES*2, g_fpLog);
				fflush(g_fpLog);
				}
		#endif

		nSiz = aacEnc_frame_with_adts_head(pBufPCM, AAC_ENC_SIZES<<1, pBufAAC, &nLenAfterEnc); //pBufAAC: 1K
		if(nSiz >0) {
// 			if(0xFF != (HINT32)(pBufAAC[0] & 0xFF)) {
// 				aacEnc_makeHead((HCHAR *)pBufAAC, gs_nAudioSampleRate, nLenAfterEnc + 7);
// 				printf("    aacEnc_frame(.)=%d srcLen=%d nDstLen=%d \n", nSiz, AAC_ENC_SIZES, nLenAfterEnc);
// 			}else aacEnc_updateHead((HCHAR *)pBufAAC, nLenAfterEnc + 7);
// 			nSiz=nLenAfterEnc+7;
			nSiz=nLenAfterEnc;

		}else{
			printf("    aacEnc_frame(.)=%d nLenAfterEnc=%d \n", nSiz, nLenAfterEnc);
			nSiz=-13;
		}
	}
	return nSiz;
}

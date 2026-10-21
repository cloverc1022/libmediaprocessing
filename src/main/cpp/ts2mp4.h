
#ifndef TS2MP4_H
#define TS2MP4_H

#define  BUF_SIZ_G711_TO_AAC 4096
#define  G711_DATA_SIZES	 1024
#define  AAC_ENC_SIZES		 1024

#ifdef __cplusplus
extern "C"{
#endif

	int64_t calcuVideoTS(int64_t pts, int64_t *pnLastTSofVideo, int *pnCountVideo, int64_t *pnReferTSVideo, int64_t *pnDiffOfVideo,int64_t *pnCurTsOfVideo, int64_t *pnLastCurTsOfVideo);
	int64_t calcuAudioTS(int64_t pts, int64_t *pnLastTSofAudio, int *pnCountAudio, int64_t *pnDiffOfAudio, int64_t* pnReferTSAudio, int64_t *pnLastCurTsOfAudio, int64_t *pnCurTsOfAudio);
	int decG711toPCM(UCHAR *pG711, int nSizG711, UCHAR *pOut);
	int encG711AtoAAC(char *pDataG711, int nBytesG711, UCHAR *pBufG711, int *pnSizBufG711,UCHAR *pBufPCM, UCHAR *pBufAAC);

#ifdef __cplusplus
	}
#endif

#endif 
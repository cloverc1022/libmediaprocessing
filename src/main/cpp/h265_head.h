
#ifndef H265_HEAD_H
#define H265_HEAD_H



typedef struct vc_params_tag{
	INT32  width,height;
	UINT32 profile, level;
	UINT32 nal_length_size;
	void clear(){
		memset(this, 0, sizeof(*this));
	}
}vc_params_t;

UINT32 h265GetSps(UCHAR *pRaw, UINT32 nDataLen, INT32 bIFrame, UCHAR *pSps);
UINT32 h265GetVpsSpsPps(UCHAR *pRaw, UINT32 nDataLen, INT32 bIFrame, UCHAR *pSpsPps);
INT32  h265ParseSeqParaSet(UCHAR *data,INT32 size, vc_params_t *pParams);

#endif 
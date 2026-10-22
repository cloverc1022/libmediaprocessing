
#include "mp4_manager.h"
#include "mp4_read.h"
#include "mp4.h"

#include "mpeg-ps.h"
#include "mpeg-ts.h"
#include "mpeg-ts-proto.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "audio_aac.h"

#if defined(BUILD_FOR_OHOS) && !defined(LOGI)
#define LOGI(...)
#define LOGE(...)
#define LOGD(...)
#endif

#ifdef LINUX_JZ
	#define INT64_MIN	(-0x7fffffffffffffff - 1)
	#define INT64_MAX	0x7fffffffffffffff
#endif

#ifdef TEST_FFMPEG_31
const uint8_t ff_reverse[256] = {
	0x00,0x80,0x40,0xC0,0x20,0xA0,0x60,0xE0,0x10,0x90,0x50,0xD0,0x30,0xB0,0x70,0xF0,
	0x08,0x88,0x48,0xC8,0x28,0xA8,0x68,0xE8,0x18,0x98,0x58,0xD8,0x38,0xB8,0x78,0xF8,
	0x04,0x84,0x44,0xC4,0x24,0xA4,0x64,0xE4,0x14,0x94,0x54,0xD4,0x34,0xB4,0x74,0xF4,
	0x0C,0x8C,0x4C,0xCC,0x2C,0xAC,0x6C,0xEC,0x1C,0x9C,0x5C,0xDC,0x3C,0xBC,0x7C,0xFC,
	0x02,0x82,0x42,0xC2,0x22,0xA2,0x62,0xE2,0x12,0x92,0x52,0xD2,0x32,0xB2,0x72,0xF2,
	0x0A,0x8A,0x4A,0xCA,0x2A,0xAA,0x6A,0xEA,0x1A,0x9A,0x5A,0xDA,0x3A,0xBA,0x7A,0xFA,
	0x06,0x86,0x46,0xC6,0x26,0xA6,0x66,0xE6,0x16,0x96,0x56,0xD6,0x36,0xB6,0x76,0xF6,
	0x0E,0x8E,0x4E,0xCE,0x2E,0xAE,0x6E,0xEE,0x1E,0x9E,0x5E,0xDE,0x3E,0xBE,0x7E,0xFE,
	0x01,0x81,0x41,0xC1,0x21,0xA1,0x61,0xE1,0x11,0x91,0x51,0xD1,0x31,0xB1,0x71,0xF1,
	0x09,0x89,0x49,0xC9,0x29,0xA9,0x69,0xE9,0x19,0x99,0x59,0xD9,0x39,0xB9,0x79,0xF9,
	0x05,0x85,0x45,0xC5,0x25,0xA5,0x65,0xE5,0x15,0x95,0x55,0xD5,0x35,0xB5,0x75,0xF5,
	0x0D,0x8D,0x4D,0xCD,0x2D,0xAD,0x6D,0xED,0x1D,0x9D,0x5D,0xDD,0x3D,0xBD,0x7D,0xFD,
	0x03,0x83,0x43,0xC3,0x23,0xA3,0x63,0xE3,0x13,0x93,0x53,0xD3,0x33,0xB3,0x73,0xF3,
	0x0B,0x8B,0x4B,0xCB,0x2B,0xAB,0x6B,0xEB,0x1B,0x9B,0x5B,0xDB,0x3B,0xBB,0x7B,0xFB,
	0x07,0x87,0x47,0xC7,0x27,0xA7,0x67,0xE7,0x17,0x97,0x57,0xD7,0x37,0xB7,0x77,0xF7,
	0x0F,0x8F,0x4F,0xCF,0x2F,0xAF,0x6F,0xEF,0x1F,0x9F,0x5F,0xDF,0x3F,0xBF,0x7F,0xFF,
};


const uint8_t ff_log2_tab[256]={
	0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
	5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
	6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
	6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
};
#endif


enum HEVCNALUnitType_E{ //Table 7-1 �C NAL unit type codes and NAL unit type classes in T-REC-H.265-201802
	HEVC_NAL_TRAIL_N    = 0,
	HEVC_NAL_TRAIL_R    = 1,
	HEVC_NAL_TSA_N      = 2,
	HEVC_NAL_TSA_R      = 3,
	HEVC_NAL_STSA_N     = 4,
	HEVC_NAL_STSA_R     = 5,
	HEVC_NAL_RADL_N     = 6,
	HEVC_NAL_RADL_R     = 7,
	HEVC_NAL_RASL_N     = 8,
	HEVC_NAL_RASL_R     = 9,
	HEVC_NAL_VCL_N10    = 10,
	HEVC_NAL_VCL_R11    = 11,
	HEVC_NAL_VCL_N12    = 12,
	HEVC_NAL_VCL_R13    = 13,
	HEVC_NAL_VCL_N14    = 14,
	HEVC_NAL_VCL_R15    = 15,
	HEVC_NAL_BLA_W_LP   = 16,
	HEVC_NAL_BLA_W_RADL = 17,
	HEVC_NAL_BLA_N_LP   = 18,
	HEVC_NAL_IDR_W_RADL = 19,
	HEVC_NAL_IDR_N_LP   = 20,
	HEVC_NAL_CRA_NUT    = 21,
	HEVC_NAL_IRAP_VCL22 = 22,
	HEVC_NAL_IRAP_VCL23 = 23,
	HEVC_NAL_RSV_VCL24  = 24,
	HEVC_NAL_RSV_VCL25  = 25,
	HEVC_NAL_RSV_VCL26  = 26,
	HEVC_NAL_RSV_VCL27  = 27,
	HEVC_NAL_RSV_VCL28  = 28,
	HEVC_NAL_RSV_VCL29  = 29,
	HEVC_NAL_RSV_VCL30  = 30,
	HEVC_NAL_RSV_VCL31  = 31,
	HEVC_NAL_VPS        = 32,
	HEVC_NAL_SPS        = 33,
	HEVC_NAL_PPS        = 34,
	HEVC_NAL_AUD        = 35,
	HEVC_NAL_EOS_NUT    = 36,
	HEVC_NAL_EOB_NUT    = 37,
	HEVC_NAL_FD_NUT     = 38,
	HEVC_NAL_SEI_PREFIX = 39,
	HEVC_NAL_SEI_SUFFIX = 40,
	HEVC_NAL_RSV_NVCL41 = 41,
	HEVC_NAL_RSV_NVCL42 = 42,
	HEVC_NAL_RSV_NVCL43 = 43,
	HEVC_NAL_RSV_NVCL44 = 44,
	HEVC_NAL_RSV_NVCL45 = 45,
	HEVC_NAL_RSV_NVCL46 = 46,
	HEVC_NAL_RSV_NVCL47 = 47,
	HEVC_NAL_UNSPEC48   = 48,
	HEVC_NAL_UNSPEC49   = 49,
	HEVC_NAL_UNSPEC50   = 50,
	HEVC_NAL_UNSPEC51   = 51,
	HEVC_NAL_UNSPEC52   = 52,
	HEVC_NAL_UNSPEC53   = 53,
	HEVC_NAL_UNSPEC54   = 54,
	HEVC_NAL_UNSPEC55   = 55,
	HEVC_NAL_UNSPEC56   = 56,
	HEVC_NAL_UNSPEC57   = 57,
	HEVC_NAL_UNSPEC58   = 58,
	HEVC_NAL_UNSPEC59   = 59,
	HEVC_NAL_UNSPEC60   = 60,
	HEVC_NAL_UNSPEC61   = 61,
	HEVC_NAL_UNSPEC62   = 62,
	HEVC_NAL_UNSPEC63   = 63
};

//----{{IsLegalAPIforMP4-----------------------
#ifdef WIN32DLL
	#include "tchar.h"
#endif

#if defined(LINUX_ANDROID) && !defined(BUILD_FOR_OHOS)
	#include <dlfcn.h>
	#include "SEMP4_android.h"

#elif defined(LINUX_iOS)
    #ifdef __cplusplus
    extern "C" {
    #endif // __cplusplus
        UINT32 SEP2P_GetSDKVersion(CHAR *chDesc, INT32 nDescMaxSize);
    #ifdef __cplusplus
    }
    #endif // __cplusplus

#elif defined(LINUX)
    #ifdef __cplusplus
    extern "C" {
    #endif // __cplusplus
        UINT32 SEP2P_GetSDKVersion(CHAR *chDesc, INT32 nDescMaxSize);
    #ifdef __cplusplus
    }
    #endif // __cplusplus
#endif

#define FINGER_PRINT		("XQVBGBVXKL1PGIULHITNXY0JXZLLGIZLGILR2CNLMBYV")
typedef UINT32 (*fn_SEP2P_GetSDKVersion)(CHAR *chDesc, INT32 nDescMaxSize);
static CHAR  g_chPackageName[200]={0};
static INT32 g_nAndroidBuildVersion=0;
static AAC_ENC_CONTEXT* gs_pEncCtx=NULL;


char IsLegalAPIforMP4()
{
    /*
	char chDesc[256]={0}, chLibPath[256]={0};
	char nRet=0;
#ifdef WIN32DLL
	static TCHAR szAppPath[MAX_PATH-32] ={0};
	TCHAR szModuleName[MAX_PATH]={0};
	HMODULE hModule=NULL;
	{
		if(szAppPath[0]=='\0'){
			if(GetModuleFileName(NULL, szAppPath, MAX_PATH-32)==0) return 0;
			*(_tcsrchr(szAppPath, '\\') + 1) = 0;
		}
		_stprintf(szModuleName, _T("%s\\SEP2P_AppSDK.dll"), szAppPath);
		hModule = LoadLibrary(szModuleName);
		if(NULL == hModule) return 0;

		fn_SEP2P_GetSDKVersion fn = (fn_SEP2P_GetSDKVersion)GetProcAddress(hModule, "SEP2P_GetSDKVersion");
		if(fn==NULL) {
			nRet=0;
			goto FUN_END;
		}
		
		(fn)(chDesc, 256);
		if(chDesc[0]=='\0') {
			nRet=0;
			goto FUN_END;
		}
FUN_END:
		if(hModule) FreeLibrary(hModule);
	}
#endif

#if defined(LINUX_ANDROID)
	fn_SEP2P_GetSDKVersion fun=NULL;
	//void* handle=dlopen("./libSEP2P_AppSDK.so",RTLD_LAZY);
	sprintf(chLibPath,"/data/data/%s/lib/libSEP2P_AppSDK.so", g_chPackageName);
	void* handle=dlopen(chLibPath, RTLD_LAZY);
	//LOGI("dlopen=%d\n", (UINT32)handle);
	if(!handle) return 0;
	fun  = (fn_SEP2P_GetSDKVersion)dlsym(handle,"SEP2P_GetSDKVersion");
	if(fun==NULL){
		dlclose(handle);
		return 0;
	}
	(*fun)(chDesc, 256);
	if(handle) dlclose(handle);
	//LOGI("chDesc=%s\n", chDesc);

#elif defined(LINUX_iOS)
	SEP2P_GetSDKVersion(chDesc, 256);
#elif defined(LINUX)
    SEP2P_GetSDKVersion(chDesc, 256);
#endif

	if(strstr(chDesc, FINGER_PRINT)==NULL) nRet=0;
	else nRet=1;
	return nRet;
    */
    return 1;
}
//----}}IsLegalAPIforMP4-----------------------

UINT32 SEMP4_GetSdkVer(char *chDesc, INT32 nDescMaxSize)
{
	return MP4::SEMP4_GetSdkVer(chDesc, nDescMaxSize);
}

UINT32 SEMP4_Init(CHAR *pStrAndroidPackageName, INT32 nAndroidBuildVersion)
{
	if(pStrAndroidPackageName==NULL) g_chPackageName[0]='\0';
	else {
		memset(g_chPackageName, 0, sizeof(g_chPackageName));
		strcpy(g_chPackageName, pStrAndroidPackageName);
	}
	g_nAndroidBuildVersion=nAndroidBuildVersion;
    av_register_all();
	return 0;
}

void   SEMP4_Uninit()
{
}

INT32  SEMP4_PreGetWHFromIFrame(UCHAR* in_pData, INT32 nDataSize, INT32* in_out_array, INT32 in_nArraySize)
{
	return MP4::PreGetWHFromIFrame(in_pData, nDataSize, in_out_array, in_nArraySize);
}

UINT32 SEMP4_Create(UCHAR **ppHandle)
{
// 	if(!IsLegalAPIforMP4()) {
// 		*ppHandle=NULL;
// 		return -1;
// 	}

	MP4 *pMP4 = new MP4();
	*ppHandle = (UCHAR *)pMP4;
	return 0;
}

void   SEMP4_Destroy(UCHAR **ppHandle)
{
	MP4 *pMP4 = (MP4 *) *ppHandle;
	if(pMP4) {
		delete pMP4;
		pMP4 = NULL;
		*ppHandle=NULL;
	}
}


#define AAC_ADTS_HEAD_LEN	7
static INT32 g_nSampleRate[] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350};
INT32 SEMP4_AppendAACHead(UCHAR *pHandle,CHAR *pBufOut, INT32 nSampleRate, CHAR *pAudioData,INT32 iSrcPos, INT32 nSizeOfES)
{
	if(pHandle==NULL) return -1;
	MP4 *pMP4 = (MP4 *)pHandle;

	int nProfile = 2; // AAC LC
	int nChanCfg = 1;
	if(pMP4->m_nFreqIdx<0){
		int i=0;
		for(i=0; i<sizeof(g_nSampleRate)/sizeof(int); i++){
			if(nSampleRate==g_nSampleRate[i]){
				pMP4->m_nFreqIdx=i;
				break;
			}
		}
	}

	// fill in ADTS data
	pBufOut[0] = (UCHAR)0xFF;
	pBufOut[1] = (UCHAR)0xF9;
	pBufOut[2] = (UCHAR)(((nProfile - 1) << 6) + (pMP4->m_nFreqIdx << 2) + (nChanCfg >> 2));
	pBufOut[3] = (UCHAR)(((nChanCfg & 3) << 6) + (nSizeOfES >> 11));
	pBufOut[4] = (UCHAR)((nSizeOfES & 0x7FF) >> 3);
	pBufOut[5] = (UCHAR)(((nSizeOfES & 7) << 5) + 0x1F);
	pBufOut[6] = (UCHAR)0xFC;

	memcpy(&pBufOut[AAC_ADTS_HEAD_LEN], pAudioData+iSrcPos, nSizeOfES);
	return (AAC_ADTS_HEAD_LEN+nSizeOfES);
}

INT32  SEMP4_OpenMp4File(UCHAR *pHandle, char *pFilename, INT32 nVideoWidth, INT32 nVideoHeigh, INT32 nVideoType,
						 INT32 nAudioSampleRate, INT32 nAudioChannel, INT32 nAudioType, INT32 nRotateVideo) //h264 or mjpeg video
{
	if(pHandle==NULL) return -1;
	MP4 *pMP4 = (MP4 *)pHandle;
	return pMP4->OpenMp4File(pFilename, nVideoWidth, nVideoHeigh, nVideoType, nAudioSampleRate, nAudioChannel, nAudioType, nRotateVideo);
}

INT32  SEMP4_CloseMp4File(UCHAR *pHandle)
{
	if(pHandle==NULL) return -1;
	MP4 *pMP4 = (MP4 *)pHandle;
	INT32 nRet=pMP4->CloseMp4File();
    return nRet;
}

INT32  SEMP4_AddVideoFrame(UCHAR *pHandle, UCHAR *pFrameBuffer,INT32 iSrcPos, INT32 nFrameLen, UINT32 nTimstamp, INT32 bKeyFrame)
{
	if(pHandle==NULL) return -1;
	MP4 *pMP4 = (MP4 *)pHandle;
	INT32 nRet=pMP4->AddVideoFrame(pFrameBuffer,iSrcPos, nFrameLen, nTimstamp, bKeyFrame);
    return nRet;
}

INT32 SEMP4_AddAudioFrame(UCHAR *pHandle, UCHAR *pFrameBuffer,INT32 iSrcPos, INT32 nFrameLen, UINT32 nTimstamp)
{
	if(pHandle==NULL) return -1;
	MP4 *pMP4 = (MP4 *)pHandle;
	INT32 nRet=pMP4->AddAudioFrame(pFrameBuffer, iSrcPos, nFrameLen, nTimstamp);
	return nRet;
}
INT32 SEMP4_AddAudioFrame_ts(UCHAR *pHandle, UCHAR *pFrameBuffer,INT32 iSrcPos, INT32 nFrameLen, UINT32 nTimstamp)
{
	if(pHandle==NULL) return -1;
	MP4 *pMP4 = (MP4 *)pHandle;
	INT32 nRet=pMP4->AddAudioFrame_ts(pFrameBuffer, iSrcPos, nFrameLen, nTimstamp);
	return nRet;
}

//----{{ts to mp4-----------------------------------------
static CHAR    gs_bFoundFirst_IFrame=0;
static UCHAR   *g_pMp4=NULL;

static int g_nCountVideo=0;
static int64_t g_nReferTSVideo=0LL, g_nLastTSofVideo=0LL,g_nDiffOfVideo=0LL, g_nCurTsOfVideo=0LL, g_nLastCurTsOfVideo=0LL;
static int g_nCountAudio=0;
static int64_t  g_nReferTSAudio=0LL,g_nLastTSofAudio=0LL,g_nDiffOfAudio=0LL, g_nCurTsOfAudio=0LL, g_nLastCurTsOfAudio=0LL;

static UINT32  g_nFileNo=0;
static UCHAR   g_bAudio=0;
static CHAR    gs_bVideoTsFirst=1;
static CHAR    gs_bAudioTsFirst=1;

extern INT32 SEMP4_AddAudioFrame_ts(UCHAR *pHandle, UCHAR *pFrameBuffer,INT32 iSrcPos, INT32 nFrameLen, UINT32 nTimstamp);

static void ResetTsTimestampState(void)
{
	gs_bVideoTsFirst=1;
	gs_bAudioTsFirst=1;
	g_nCountVideo=0;
	g_nReferTSVideo=0LL;
	g_nLastTSofVideo=0LL;
	g_nDiffOfVideo=0LL;
	g_nCurTsOfVideo=0LL;
	g_nLastCurTsOfVideo=0LL;
	g_nCountAudio=0;
	g_nReferTSAudio=0LL;
	g_nLastTSofAudio=0LL;
	g_nDiffOfAudio=0LL;
	g_nCurTsOfAudio=0LL;
	g_nLastCurTsOfAudio=0LL;
}

static int64_t calcuStreamTS(int64_t pts, CHAR *pbFirst, int *pnCount, int nMaxDiffSamples,
	int64_t *pnLastPtsMs, int64_t *pnDiffMs, int64_t *pnCurTs, int64_t *pnLastCurTs)
{
	int64_t ptsMs=pts/90;
	int64_t nCurTs=0LL;

	if(*pbFirst){
		*pnLastPtsMs=ptsMs;
		*pnLastCurTs=0LL;
		nCurTs=0LL;
		*pbFirst=0;
	}else{
		int64_t nTmp=ptsMs - *pnLastPtsMs;
		// 分片边界或异常 PTS（副路 G711 常见巨跳）：勿把超大正向 delta 写入 mp4，否则音轨 duration 爆炸
		const int64_t kMaxPtsStepMs = 3000;
		if(nTmp<0 || nTmp>kMaxPtsStepMs){
			nCurTs=*pnLastCurTs + (*pnDiffMs>0 ? *pnDiffMs : 1);
		}else if(nTmp==0){
			nCurTs=*pnLastCurTs + (*pnDiffMs>0 ? *pnDiffMs : 1);
		}else{
			nCurTs=*pnLastCurTs + nTmp;
		}
		if(*pnCount<nMaxDiffSamples && nTmp>0){
			*pnDiffMs=nTmp;
			(*pnCount)++;
		}
	}

	*pnLastPtsMs=ptsMs;
	*pnCurTs=nCurTs;
	*pnLastCurTs=nCurTs;
	return nCurTs;
}

int64_t calcuVideoTS(int64_t pts)
{
	return calcuStreamTS(pts, &gs_bVideoTsFirst, &g_nCountVideo, 3,
		&g_nLastTSofVideo, &g_nDiffOfVideo, &g_nCurTsOfVideo, &g_nLastCurTsOfVideo);
}
int64_t calcuAudioTS(int64_t pts)
{
	return calcuStreamTS(pts, &gs_bAudioTsFirst, &g_nCountAudio, 5,
		&g_nLastTSofAudio, &g_nDiffOfAudio, &g_nCurTsOfAudio, &g_nLastCurTsOfAudio);
}

const char* ftimestamp(int64_t t, char* buf){
	if(PTS_NO_VALUE == t) sprintf(buf, "(null)");
	else{
		t /= 90;
		sprintf(buf, "%d:%02d:%02d.%03d", (int)(t / 3600000), (int)((t / 60000) % 60), (int)((t / 1000) % 60), (int)(t % 1000));
	}
	return buf;
}

#define  BUF_SIZ_G711_TO_AAC 4096
#define  G711_DATA_SIZES	 1024
#define  AAC_ENC_SIZES		 1024

static int gs_eDetectVEnc=RTSP_AV_CODECID_UNKNOWN; //0
static int gs_eDetectAEnc=RTSP_AV_CODECID_UNKNOWN;

static int gs_nSizBufG711=0;
static HUCHAR *gs_pBufG711=NULL, *gs_pBufPCM=NULL, *gs_pBufAAC=NULL;
static int64_t gs_nCurTsForG711=0LL;

//FILE *g_fpLog=NULL;
//FILE *fpVideo=NULL;	
void writeVideoFrm(char *pData, int nSize){
	static int nCount=0;
	/*
	FILE *fpVideo=NULL;	
	char sFilename[64]={0};
	sprintf(sFilename, "D:\\ts2\\h265\\v_frm\\%04d.h265", nCount++);
	fpVideo=fopen(sFilename, "wb");
	if(fpVideo){
		fwrite(pData, 1, nSize, fpVideo);
		fflush(fpVideo);
		fclose(fpVideo);
	}
	*/
// 	if(fpVideo==NULL) {
// 		char sFilename[64] = { 0 };
// 		sprintf(sFilename, "D:\\ts2\\h264\\1660384349\\h264\\one_%04d.h264", nCount++);
// 		fpVideo=fopen(sFilename, "wb");
// 	}
// 	if(fpVideo){
// 		fwrite(pData, 1, nSize, fpVideo);
// 		fflush(fpVideo);
// 	}
}


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
int decG711toPCM(HUCHAR *pG711, int nSizG711, HUCHAR *pOut){
	return g711a_Decode(pG711, nSizG711, pOut, NULL);
}

//return:
//	-12: the data is too little.
//	-13: failed to aacEnc_frame_with_adts_head
//
static int encG711AtoAAC(char *pDataG711, int nBytesG711){
	int nSiz=0;

	if(nBytesG711 > 0) {
		memcpy(gs_pBufG711 + gs_nSizBufG711, pDataG711, nBytesG711);
		gs_nSizBufG711 += nBytesG711;
	}
	if(gs_nSizBufG711 < G711_DATA_SIZES) {
		return -12;
	}

	int nLenAfterEnc=AAC_ENC_SIZES;
	decG711toPCM(gs_pBufG711, G711_DATA_SIZES, gs_pBufPCM);
	memcpy(gs_pBufG711, gs_pBufG711+G711_DATA_SIZES,  gs_nSizBufG711-G711_DATA_SIZES);
	gs_nSizBufG711 -= G711_DATA_SIZES;

	nSiz = aacEnc_frame_with_adts_head(gs_pEncCtx,gs_pBufPCM, AAC_ENC_SIZES<<1, gs_pBufAAC, &nLenAfterEnc);
	if(nSiz >0) {
		nSiz=nLenAfterEnc;
	}else{
		printf("    aacEnc_frame(.)=%d nLenAfterEnc=%d \n", nSiz, nLenAfterEnc);
		nSiz=-13;
	}
	return nSiz;
}

static int flushG711RemainderForTs2Mp4(){
	if(gs_eDetectAEnc != RTSP_AV_CODECID_AUDIO_G711A) return 0;
	if(gs_nSizBufG711 <= 0 || g_pMp4 == NULL || gs_pEncCtx == NULL) return 0;

	while(gs_nSizBufG711 > 0 && gs_nSizBufG711 < G711_DATA_SIZES){
		gs_pBufG711[gs_nSizBufG711++] = (UCHAR)0xD5;
	}
	if(gs_nSizBufG711 < G711_DATA_SIZES) return 0;

	int nLenAfterEnc = AAC_ENC_SIZES;
	decG711toPCM(gs_pBufG711, G711_DATA_SIZES, gs_pBufPCM);
	int nAacLen = aacEnc_frame_with_adts_head(gs_pEncCtx, gs_pBufPCM, AAC_ENC_SIZES<<1, gs_pBufAAC, &nLenAfterEnc);
	gs_nSizBufG711 = 0;
	if(nAacLen <= 0) return -100;

	UINT32 nTs = (UINT32)(gs_nCurTsForG711 != 0LL ? gs_nCurTsForG711 : g_nCurTsOfAudio);
	if(nTs == 0) nTs = 1;
	return SEMP4_AddAudioFrame_ts(g_pMp4, (UCHAR *)gs_pBufAAC, 0, nLenAfterEnc, nTs);
}

static int on_ts_packet(void* /*param*/, int program, int /*stream*/, int avtype, int flags, int64_t pts, int64_t dts, const void* data, size_t bytes){
	static char s_pts[64]={0}, s_dts[64]={0};
	int64_t nCurTs=0LL;
	
	#ifdef LINUX_ANDROID
		LOGI("on_ts_packet avtype=%d flags=%d pts=%08d dts=%08d size=%08d gs_eDetectVEnc=%d\n", avtype, flags, pts,  dts, bytes, gs_eDetectVEnc);
	#endif

	int nRet=0;
	if(PSI_STREAM_AAC == avtype || PSI_STREAM_AUDIO_G711A==avtype || PSI_STREAM_AUDIO_G711U==avtype || PSI_STREAM_PRIVATE_DATA==avtype){
		static int64_t a_pts = 0, a_dts = 0;
		if (PTS_NO_VALUE == dts) dts = pts;				
		//assert(0 == a_dts || dts >= a_dts);

		if(gs_eDetectVEnc > RTSP_AV_CODECID_UNKNOWN && gs_eDetectAEnc > RTSP_AV_CODECID_UNKNOWN){
			if(gs_bFoundFirst_IFrame && g_bAudio){
				nCurTs=calcuAudioTS(pts);
				if(g_pMp4){
					if(gs_eDetectAEnc==RTSP_AV_CODECID_AUDIO_G711A){
						if(gs_nCurTsForG711==0LL) gs_nCurTsForG711=nCurTs;
						nRet=encG711AtoAAC((char *)data, bytes); //һ֡��g711a���ݱ���<1024bytes
						if(nRet>0) {
							nRet=SEMP4_AddAudioFrame_ts(g_pMp4, (UCHAR *)gs_pBufAAC, 0, nRet, gs_nCurTsForG711);
							gs_nCurTsForG711=0LL;
						}

					}else nRet=SEMP4_AddAudioFrame_ts(g_pMp4, (UCHAR *)data, 0, bytes, nCurTs);
				}
				if(nRet!=0) printf("err, SEMP4_AddAudioFrame_ts(.)=%d \n", nRet);
				
				//printf("AAC a_pts: %s(%lld), dts: %s(%lld), diff: %03d/%03d  nCurTs=%lld\n", ftimestamp(pts, s_pts), pts, ftimestamp(dts, s_dts), dts, (int)(pts - a_pts) / 90, (int)(dts - a_dts) / 90, nCurTs);
				a_pts = pts;
				a_dts = dts;
			}
		}
	}else if (PSI_STREAM_H264 == avtype){
		int flagsFromFrm=0;
		static int64_t v_pts = 0, v_dts = 0;		

		char *pRawData=(char *)data;
		int nNalType=0;
		//printf("0x%02X %02X %02X %02X %02X %02X %02X \n", pRawData[0]&0xFF,pRawData[1]&0xFF,pRawData[2]&0xFF,pRawData[3]&0xFF,pRawData[4]&0xFF,pRawData[5]&0xFF,pRawData[6]&0xFF);
		nNalType=pRawData[4]&0x1F;
		if((pRawData[2]&0xFF)==0 && (pRawData[3]&0xFF)==1 && (nNalType==7 ||nNalType==8)){
			flagsFromFrm=1;
		}
		//assert(0 == v_dts || dts >= v_dts);
		#ifdef LINUX_ANDROID
			LOGI("nNalType=%d flagsFromFrm=%d size=%08d pts=%lld\n", nNalType, flagsFromFrm, bytes,  pts);
		#endif
		//fwrite(data, 1, bytes, vfp);
		if(!gs_bFoundFirst_IFrame && flagsFromFrm) gs_bFoundFirst_IFrame=1;		
		if(gs_bFoundFirst_IFrame){
			if(gs_eDetectVEnc>RTSP_AV_CODECID_UNKNOWN){
				nCurTs=calcuVideoTS(pts);
				if(g_pMp4) nRet=SEMP4_AddVideoFrame(g_pMp4, (UCHAR *)data, 0, bytes, nCurTs, flagsFromFrm);
// 				writeVideoFrm((char *)data, bytes);
// 
// 				printf("H264 v_pts: %s(%lld) v_pts=%lld, dts: %s(%lld) v_dts=%lld, diff: %03d/%03d, %d flag=%d nCurTs=%lld\n", ftimestamp(pts, s_pts), pts, v_pts, ftimestamp(dts, s_dts), dts, v_dts, (int)(pts - v_pts) / 90,
// 					   (int)(dts - v_dts) / 90, flagsFromFrm, flags, nCurTs);
				v_pts = pts;
				v_dts = dts;
		}
		}
	}else if(PSI_STREAM_H265 == avtype){
		#define TS_H265_NAL_TYPE(v)	((v>>1) & 0x3f)
		//#define IMP_H265_NAL_SLICE_BLA_W_LP  16
		//#define IMP_H265_NAL_SLICE_CRA       21

		int flagsFromFrm=0;
		static int64_t v_pts = 0, v_dts = 0;		

		char *pRawData=(char *)data;
		int nNalType=0;
		nNalType=TS_H265_NAL_TYPE(pRawData[4]);
		if(nNalType==HEVC_NAL_VPS || (nNalType >= HEVC_NAL_BLA_W_LP && nNalType <= HEVC_NAL_CRA_NUT)) flagsFromFrm=1;

		//if((pRawData[4] & 0x7e) == 0x40 || (pRawData[4] & 0x7e) == 0x42 || (pRawData[4] & 0x7e) == 0x44 || (pRawData[4] & 0x7e) == 0x4e) flagsFromFrm=0;
		//else flagsFromFrm=1;
		#ifdef LINUX_ANDROID
			LOGI("nNalType=%d flagsFromFrm=%d size=%08d pts=%lld gs_eDetectVEnc=%d\n", nNalType, flagsFromFrm, bytes,  pts, gs_eDetectVEnc);
		#endif

		//assert(0 == v_dts || dts >= v_dts);
		//printf("nNalType=%d flagsFromFrm=%d size=%08d pts=%lld\n", nNalType, flagsFromFrm, bytes,  pts);

		//writeVideoFrm((char *)data, bytes);

		//fwrite(data, 1, bytes, vfp);
		if(!gs_bFoundFirst_IFrame && flagsFromFrm) gs_bFoundFirst_IFrame=1;
		if(gs_bFoundFirst_IFrame){
			if(gs_eDetectVEnc>RTSP_AV_CODECID_UNKNOWN){	
				nCurTs=calcuVideoTS(pts);
				if(g_pMp4) nRet=SEMP4_AddVideoFrame(g_pMp4, (UCHAR *)data, 0, bytes, nCurTs, flagsFromFrm);
				#ifdef LINUX_ANDROID
					LOGI("H265 pts: %s(%lld) v_pts=%lld, dts: %s(%lld) v_dts=%lld, diff: %03d/%03d, %d flag=%d nCurTs=%lld, size=%08d, %d\n", ftimestamp(pts, s_pts), pts,v_pts, ftimestamp(dts, s_dts), dts,v_dts, (int)(pts - v_pts) / 90, 
					(int)(dts - v_dts) / 90, flagsFromFrm, flags, nCurTs, bytes, nRet);
				#endif
				//unsigned char *pD = (unsigned char *)data;
				//printf("----bytes=%05d %d, 0x%02X 0x%02X 0x%02X 0x%02X  0x%02X 0x%02X 0x%02X 0x%02X\n", bytes, flagsFromFrm, 
				//		pD[0]&0xFF,pD[1]&0xFF,pD[2]&0xFF,pD[3]&0xFF, pD[4]&0xFF, pD[5]&0xFF, pD[6]&0xFF, pD[7]&0xFF);

// 				printf("H265 v_pts: %s(%lld) v_pts=%lld, dts: %s(%lld) v_dts=%lld, diff: %03d/%03d, %d flag=%d nCurTs=%lld\n", ftimestamp(pts, s_pts), pts,v_pts, ftimestamp(dts, s_dts), dts,v_dts, (int)(pts - v_pts) / 90, 
// 					(int)(dts - v_dts) / 90, flagsFromFrm, flags, nCurTs);
				v_pts = pts;
				v_dts = dts;
			}else{
				gs_eDetectVEnc=RTSP_AV_CODECID_VIDEO_H265;
			}
		}

	}else{
		//assert(0);
	}
	return 0;
}

static int on_ts_packet_detectVEnc(void* /*param*/, int program, int /*stream*/, int avtype, int flags, int64_t pts, int64_t dts, const void* data, size_t bytes){
	
	#ifdef LINUX_ANDROID
		LOGI("on_ts_packet_detectVEnc avtype=%d flags=%d pts=%08d dts=%08d size=%08d\n", avtype, flags, pts,  dts, bytes);
	#endif

	int nRet=0;
	if(PSI_STREAM_AAC == avtype || PSI_STREAM_AUDIO_G711A==avtype || PSI_STREAM_AUDIO_G711U==avtype || PSI_STREAM_PRIVATE_DATA==avtype){
		if(gs_eDetectVEnc > RTSP_AV_CODECID_UNKNOWN && gs_eDetectAEnc > RTSP_AV_CODECID_UNKNOWN){
			
		}else{
			switch(avtype){
				case PSI_STREAM_AUDIO_G711A:
				case PSI_STREAM_AUDIO_G711U:
				case PSI_STREAM_PRIVATE_DATA:
					gs_eDetectAEnc=RTSP_AV_CODECID_AUDIO_G711A;
					break;
				case PSI_STREAM_AAC:
					gs_eDetectAEnc=RTSP_AV_CODECID_AUDIO_AAC;
					break;
				default:;
			}
		}
	}else if (PSI_STREAM_H264 == avtype){
			if(gs_eDetectVEnc>RTSP_AV_CODECID_UNKNOWN){
			}else{
				gs_eDetectVEnc=RTSP_AV_CODECID_VIDEO_H264;
			}
	}else if(PSI_STREAM_H265 == avtype){
			if(gs_eDetectVEnc>RTSP_AV_CODECID_UNKNOWN){	

			}else{
				gs_eDetectVEnc=RTSP_AV_CODECID_VIDEO_H265;
			}
	}else{
		//assert(0);
	}
	return 0;
}

static int read_ts_packet_resync(FILE *fp, UCHAR *chBuf, size_t nBufSize)
{
	if (fp == NULL || chBuf == NULL || nBufSize < 188) return 0;

	int ch = 0;
	while ((ch = fgetc(fp)) != EOF) {
		if (ch != 0x47) continue;

		long packetStart = ftell(fp);
		if (packetStart >= 1) packetStart -= 1;

		chBuf[0] = (UCHAR) ch;
		if (fread(chBuf + 1, 1, 187, fp) != 187) return 0;

		long nextPacketStart = ftell(fp);
		int next = fgetc(fp);
		if (next == EOF) return 1;

		if (0 == fseek(fp, nextPacketStart, SEEK_SET) && next == 0x47) {
			return 1;
		}

		if (packetStart < 0 || 0 != fseek(fp, packetStart + 1, SEEK_SET)) {
			return 0;
		}
	}

	return 0;
}

//����ļ���ֻ��� 1st �ļ�
INT32 TS2MP4_detectVEnc(CHAR *pTsFilelistBytime)
{
	INT32 nRet=-1;
	const CHAR cSplit[]=",";
	CHAR  cTsFiles[512]={0}; 
	CHAR  *pFound=NULL;
	FILE *fp=NULL;
	UCHAR chBuf[188]={0};

	ts_demuxer_t *pTS=NULL;
	pTS = ts_demuxer_create(on_ts_packet_detectVEnc, NULL);
	if(pTS==NULL) {
		nRet =-5;
		goto APP_END;
	}
	
	strncpy(cTsFiles, pTsFilelistBytime, sizeof(cTsFiles)-1);
	pFound=strtok(cTsFiles, cSplit);
	if(pFound==NULL) goto APP_END;
	fp=fopen(pFound, "rb");
	if(fp==NULL) goto APP_END;

	while(read_ts_packet_resync(fp, chBuf, sizeof(chBuf))){
		nRet=ts_demuxer_input(pTS, chBuf, sizeof(chBuf));
		if(nRet!=0) {
			nRet=-4;
			break;
		}
		#ifdef LINUX_ANDROID
			LOGI("TS2MP4_detectVEnc video=%d audio=%d\n", gs_eDetectVEnc, gs_eDetectAEnc);
		#endif
		if(gs_eDetectVEnc>RTSP_AV_CODECID_UNKNOWN && (gs_eDetectAEnc==-1 || gs_eDetectAEnc>RTSP_AV_CODECID_UNKNOWN)) break;
	}
	#ifdef LINUX_ANDROID
			LOGI("TS2MP4_detectVEnc_end video=%d audio=%d\n", gs_eDetectVEnc, gs_eDetectAEnc);
	#endif
	if(fp){
		fclose(fp);
		fp=NULL;
	}

	ts_demuxer_flush(pTS);
	ts_demuxer_destroy(pTS);

APP_END:
	return gs_eDetectVEnc;
}
//return:
//	 0: ok
//	<0: fail
//		-1: pTsFilelistBytime==NULL || pOutFilename==NULL
//		-2: SEMP4_Create(.), pMp4==NULL
//		-3: SEMP4_OpenMp4File(.)<0
//		-4: 0!=ts_demuxer_input(.)
//		-5: ts_demuxer_create(.)==NULL
//		-6: unknown video encoding format
INT32  TS2MP4(CHAR *pTsFilelistBytime,INT32 eVideoType, INT32 eAudioType,INT32 nAudioSampleRate,INT32 nAudioChannel,  CHAR *pOutFilename, INT32 nRotateVideo){
	INT32 nRet=-1;
	FILE  *fp=NULL;
	CHAR  *pFound=NULL;
	CHAR  *pTsFilelistCopy=NULL;
	size_t nTsFilelistLen=0;
	const CHAR cSplit[]=",";
	
	UCHAR chBuf[188]={0};
	ts_demuxer_t *pTS=NULL;

	if(pTsFilelistBytime==NULL || pOutFilename==NULL) return -1;
	if(pTsFilelistBytime[0]=='\0' || pOutFilename[0]=='\0') return -1;
	nTsFilelistLen=strlen(pTsFilelistBytime);
	pTsFilelistCopy=(CHAR *)malloc(nTsFilelistLen + 1);
	if(pTsFilelistCopy==NULL) return -8;
	memcpy(pTsFilelistCopy, pTsFilelistBytime, nTsFilelistLen + 1);
	
	gs_eDetectVEnc=eVideoType;
	gs_eDetectAEnc=eAudioType;
	if(eVideoType<=RTSP_AV_CODECID_UNKNOWN){
		eVideoType=TS2MP4_detectVEnc(pTsFilelistBytime);
		if(eVideoType<0) {
			nRet=-6;
			goto APP_END;
		}
		else {
			if(gs_eDetectAEnc<RTSP_AV_CODECID_AUDIO_AAC) eAudioType=RTSP_AV_CODECID_AUDIO_AAC;
			else eAudioType=RTSP_AV_CODECID_AUDIO_AAC;
		}
	}
	g_pMp4=NULL;
	SEMP4_Create(&g_pMp4);
	if(g_pMp4==NULL) {
		nRet=-2;
		goto APP_END;
	}

	gs_nCurTsForG711=0LL;
	nRet=SEMP4_OpenMp4File(g_pMp4, pOutFilename, 0,0, eVideoType,nAudioSampleRate,nAudioChannel,eAudioType, nRotateVideo);
	if(nRet<0){
		nRet=-3;
		goto APP_END;
	}
	if(gs_pBufG711==NULL) gs_pBufG711=(HUCHAR *)malloc(G711_DATA_SIZES<<1);
	gs_nSizBufG711=0;
	if(gs_pBufPCM==NULL)  gs_pBufPCM=(HUCHAR *)malloc(G711_DATA_SIZES<<1);
	if(gs_pBufAAC==NULL)  gs_pBufAAC=(HUCHAR *)malloc(AAC_ENC_SIZES);
	if(gs_pBufAAC) memset(gs_pBufAAC, 0, AAC_ENC_SIZES);

	{
		INT32 nAacChannels = nAudioChannel <= 0 ? 1 : (nAudioChannel > 2 ? 2 : nAudioChannel);
		gs_pEncCtx=aacEnc_init(nAudioSampleRate, nAacChannels, 16, nAudioSampleRate*2);
	}
	pTS = ts_demuxer_create(on_ts_packet, NULL);
	if(pTS==NULL) {
		nRet =-5;
		goto APP_END;
	}

	gs_bFoundFirst_IFrame=0;
	ResetTsTimestampState();
	g_nFileNo=0;

	//if(g_fpLog==NULL) g_fpLog=fopen("d:\\ts2mp4.log", "wb");

	if(eAudioType>0) g_bAudio=1;
	pFound=strtok(pTsFilelistCopy, cSplit);
	while(pFound!=NULL){
		fp=fopen(pFound, "rb");
		if(fp==NULL) {
			nRet=-7;
			goto APP_END;
		}
		while(read_ts_packet_resync(fp, chBuf, sizeof(chBuf))){
			nRet=ts_demuxer_input(pTS, chBuf, sizeof(chBuf));
			if(nRet!=0) {
				nRet=-4;
				break;
			}
		}
		if(fp){
			fclose(fp);
			fp=NULL;
		}
		if(nRet!=0) {
			goto APP_END;
		}
		g_nFileNo++;
		printf("g_nFileNo=%d \n", g_nFileNo);

		pFound=strtok(NULL, cSplit);
	}

APP_END:	
	if(pTS){
		ts_demuxer_flush(pTS);
		ts_demuxer_destroy(pTS);
		pTS=NULL;
	}

	if(g_pMp4){
		if(nRet == 0) {
			int nFlush = flushG711RemainderForTs2Mp4();
			if(nFlush != 0 && nFlush != -100) {
				#ifdef LINUX_ANDROID
				LOGI("TS2MP4 flushG711Remainder ret=%d\n", nFlush);
				#endif
			}
		}
		SEMP4_CloseMp4File(g_pMp4);
		SEMP4_Destroy(&g_pMp4);
	}
	if(fp){
		fclose(fp);
		fp=NULL;
	}
	aacEnc_exit(gs_pEncCtx);
	gs_pEncCtx=NULL;
	if(gs_pBufG711){
		free(gs_pBufG711);
		gs_pBufG711=NULL;
	}
	gs_nSizBufG711=0;

	if(gs_pBufPCM) {
		free(gs_pBufPCM);
		gs_pBufPCM = NULL;
	}
	if(gs_pBufAAC) {
		free(gs_pBufAAC);
		gs_pBufAAC = NULL;
	}
	if(pTsFilelistCopy){
		free(pTsFilelistCopy);
		pTsFilelistCopy=NULL;
	}

// 	if(g_fpLog){
// 		fclose(g_fpLog);
// 		g_fpLog=NULL;
// 	}
	return nRet;
}
//----}}ts to mp4-----------------------------------------



//----{{Read MP4 API----
UINT32 SEMP4Read_Create(UCHAR **ppHandle)
{
// 	if(!IsLegalAPIforMP4()) {
// 		*ppHandle=NULL;
// 		return -1;
// 	}

	MP4Read *pMP4Read = new MP4Read();
	*ppHandle = (UCHAR *)pMP4Read;
	return 0;	
}

void   SEMP4Read_Destroy(UCHAR **ppHandle)
{
	MP4Read *pMP4Read = (MP4Read *) *ppHandle;
	if(pMP4Read) {
		delete pMP4Read;
		pMP4Read = NULL;
		*ppHandle=NULL;
	}
}

INT32  SEMP4Read_OpenMp4File(UCHAR *pHandle, char *pFilename, FILE_INFO *pFileInfo)
{
	if(pHandle==NULL) return -1;
	MP4Read *pMP4Read = (MP4Read *)pHandle;
	return pMP4Read->OpenMp4File(pFilename, pFileInfo);
}

INT32  SEMP4Read_CloseMp4File(UCHAR *pHandle)
{
	if(pHandle==NULL) return -1;
	MP4Read *pMP4Read = (MP4Read *)pHandle;
	INT32 nRet=pMP4Read->CloseMp4File();
	return nRet;
}

INT32  SEMP4Read_SetBeginPos(UCHAR *pHandle, INT32 nBeginPos_sec)
{
	if(pHandle==NULL) return -1;
	MP4Read *pMP4Read = (MP4Read *)pHandle;
	pMP4Read->SetBeginPos(nBeginPos_sec);
    return 0;
}

//pData=RTSP_STREAM_HEAD + raw_av_data
//nDataSize=sizeof(RTSP_STREAM_HEAD) + raw_av_data_size
INT32  SEMP4Read_ReadOneFrame(UCHAR *pHandle, UCHAR* pBuf, UINT32 nBufMaxSize)
{
	if(pHandle==NULL) return -1;
	MP4Read *pMP4Read = (MP4Read *)pHandle;
	INT32 nRet=pMP4Read->ReadOneFrame(pBuf, nBufMaxSize);
	return nRet;
}
//----}}Read MP4 API----




//----{{RTSP API----
SEMP4_API_API UINT32 RTSP_GetSdkVer(char *chDesc, INT32 nDescMaxSize)
{
	return RTSP::RTSP_GetSdkVer(chDesc, nDescMaxSize);
}

SEMP4_API_API UINT32 RTSP_Create(UCHAR **ppHandle, RTSP_POnStreamCallback pOnStreamCallback, void *pUserData)
{
	RTSP *pRTSP = new RTSP((RTSP::RTSP_POnStreamCallback)pOnStreamCallback, pUserData);
	*ppHandle = (UCHAR *)pRTSP;
	return 0;	
}

SEMP4_API_API void RTSP_Destroy(UCHAR **ppHandle)
{
	RTSP *pRTSP = (RTSP *) *ppHandle;
	if(pRTSP) {
		delete pRTSP;
		pRTSP = NULL;
	}
}

SEMP4_API_API INT32  RTSP_OpenUrl(UCHAR *pHandle, char *pchUrl, char *pchKeyValue)
{
	if(pHandle==NULL) return -1;
	RTSP *pRTSP = (RTSP *)pHandle;
	return pRTSP->RTSP_OpenUrl(pchUrl, pchKeyValue);
}

SEMP4_API_API INT32  RTSP_CloseUrl(UCHAR *pHandle)
{
	if(pHandle==NULL) return -1;
	RTSP *pRTSP = (RTSP *)pHandle;
	return pRTSP->RTSP_CloseUrl();
}

SEMP4_API_API INT32  RTSP_ResetUrl(UCHAR *pHandle, char *pchUrl)
{
	if(pHandle==NULL) return -1;
	RTSP *pRTSP = (RTSP *)pHandle;
	return pRTSP->RTSP_ResetUrl(pchUrl);
}

//----}}RTSP API----

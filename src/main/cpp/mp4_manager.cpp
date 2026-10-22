/*********ChangeLog*******************************************************
comment: fix, fix some bugs; +, add some function; *, modify some content
--------------------------------------------------------------------------
	2.0.16/2.0.15
		2.0.16 修复g711音频录像时，g711转aac崩溃的问题
		2.0.15
		增加宏 AUDIO_G711_2_AAC_WHEN_REC_MP4;
		当nAudioType=RTSP_AV_CODECID_AUDIO_G711A时，SEMP4_OpenMp4File(...),SEMP4_AddAudioFrame(...) 自动转换为AAC进行录制
		SEMP4_AddAudioFrame(...) 一次输入的g711a音频数据必须<=1024Bytes

	2.0.14 20230426
		calcuVideoTS/calcuAudioTS(pts), 当pts为0时，calcuVideoTS/calcuAudioTS计算结果第1，2帧都为0
	2.0.13 20221021
		修复 AAC的ts文件，转换为mp4后无声音的问题
		2.0.12 20221021
			修复 重复调用TS2MP4，空指针错误
		2.0.11 20221015, 2.0.10 20221008
			TS2MP4 api: 增加对 .ts文件音频的识别，并将g711A音频转为AAC
*************************************************************************/

#include "mp4_manager.h"
#include "mp4.h"
#include "h265_head.h"

#if defined(LINUX_ANDROID) && !defined(BUILD_FOR_OHOS)
	#include <dlfcn.h>
	#include "SEMP4_android.h"
#endif
//#include "ts2mp4.h"
#include "audio_aac.h"
#include <ctype.h>

#define FINGER_PRINT		("XQVBGBVXKL1PGIULHITNXY0JXZLLGIZLGILR2CNLMBYV")
#define FINGER_PRINT_LEN	44
//----{{IsLegalAPIforMP4-----------------------

#define MODULE_NAME				("SEP2P_AppSDK.dll")
#define MODULE_FUN_GET_VERSION	("SEP2P_GetSDKVersion")
#define AUDIO_G711_2_AAC_WHEN_REC_MP4 //mp4 录像,g711-->mp4


#ifdef WIN32DLL
	#pragma comment(lib,"libgcc.a")
	#pragma comment(lib,"libmingwex.a")
	#pragma comment(lib,"ws2_32.lib")
	#pragma comment(lib,"Advapi32.lib")
	#pragma comment(lib,"libavcodec.a")
	#pragma comment(lib,"libavformat.a")
	#pragma comment(lib,"libavutil.a")
	#pragma comment(lib,"libswresample.a")
	
	#pragma comment(lib,"bcrypt.lib")
#endif

//======================================================
#define SE_SDK_VERSION      0x02001000   //"2.0.8"
#define SE_SDK_BUILD_TIME	"2023-12-17" //"2023-04-26"


//======================================================
#define	TIME_BASE_AV		90000
#define	TIME_ST_FREQ		90

#define  BUF_SIZ_G711_TO_AAC 4096
#define  G711_DATA_SIZES	 1024
#define  AAC_ENC_SIZES		 1024


#if defined(LINUX_ANDROID) && !defined(BUILD_FOR_OHOS)
	#include "SEMP4_android.h"
	//#define TRACE	LOGI		//juju
	#define TRACE(...)

	extern  char IsLegalAPIforMP4();
#else
	//#define TRACE printf //juju2018
	#define TRACE(...)
#endif



#define	AUDIO_FRAME_SIZE 1024
static const int g_AudioSampleRates[16] = {
	96000, 88200, 64000, 48000, 44100, 32000,
	24000, 22050, 16000, 12000, 11025, 8000, 7350
};

static int ends_with_mov_ext(const char* filename)
{
	if(filename == NULL) return 0;
	const char* ext = strrchr(filename, '.');
	if(ext == NULL) return 0;
	if(ext[1] == '\0') return 0;
	char c1 = (char)tolower((unsigned char)ext[1]);
	char c2 = (char)tolower((unsigned char)ext[2]);
	char c3 = (char)tolower((unsigned char)ext[3]);
	return (c1 == 'm' && c2 == 'o' && c3 == 'v' && ext[4] == '\0') ? 1 : 0;
}

static int patch_hev1_to_hvc1_in_file(const char* filename)
{
	if(filename == NULL || filename[0] == '\0') return -1;
	FILE* fp = fopen(filename, "rb+");
	if(fp == NULL) return -2;

	const unsigned char fromTag[4] = { 'h', 'e', 'v', '1' };
	const unsigned char toTag[4] = { 'h', 'v', 'c', '1' };
	unsigned char buf[4099] = { 0 }; // 4096 + 3 bytes overlap
	size_t carry = 0;
	int replaceCount = 0;

	while(1){
		size_t nRead = fread(buf + carry, 1, 4096, fp);
		if(nRead == 0) break;
		long nextReadPos = ftell(fp);
		size_t total = carry + nRead;
		size_t i = 0;
		while(i + 3 < total){
			if(buf[i] == fromTag[0] && buf[i+1] == fromTag[1] && buf[i+2] == fromTag[2] && buf[i+3] == fromTag[3]){
				long patchPos = nextReadPos - (long)total + (long)i;
				if(fseek(fp, patchPos, SEEK_SET) == 0){
					fwrite(toTag, 1, 4, fp);
					replaceCount++;
					fseek(fp, nextReadPos, SEEK_SET);
				}
			}
			i++;
		}
		carry = total >= 3 ? 3 : total;
		if(carry > 0){
			memmove(buf, buf + total - carry, carry);
		}
	}
	fclose(fp);
	return replaceCount;
}

#ifdef WIN32DLL
#ifdef _MANAGED
	#pragma managed(push, off)
#endif

//HWND GetDesktopWindow() { return NULL; } //juju

#ifndef USE_STATIC_LIB
BOOL APIENTRY DllMain(HMODULE hModule,
					  DWORD  ul_reason_for_call,
					  LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		{
// 			WSADATA	WSAData = { 0 };
// 			if(WSAStartup(WSA_VERSION, &WSAData) != 0)
// 			{
// 				// Tell the user that we could not find a usable WinSock DLL.
// 				if (LOBYTE(WSAData.wVersion) != LOBYTE(WSA_VERSION) ||
// 					HIBYTE(WSAData.wVersion) != HIBYTE(WSA_VERSION))
// 					printf("Incorrect winsock version\n");
// 
// 				WSACleanup();
// 				return false;
// 			}
			break;
		}

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH:
		//WSACleanup();
		break;

	default:;
	}
	return TRUE;
}
#endif ////#ifndef USE_STATIC_LIB
	#ifdef _MANAGED
	#pragma managed(pop)
	#endif  //// #ifdef _MANAGED
#endif  //// #ifdef WIN32DLL


//----{{MP4----------------------------------------------------------------------------------------------
static const UCHAR* ff_avc_find_startcode_internal(const UCHAR *p, const UCHAR *end)
{
	const UCHAR *a = p + 4-((intptr_t)p & 3);

	for(end -= 3; p < a && p < end; p++){
		if(p[0] == 0 && p[1] == 0 && p[2] == 1) return p;
	}

	for(end -= 3; p < end; p += 4) {
		UINT32 x = *(const UINT32*)p;
		//      if ((x - 0x01000100) & (~x) & 0x80008000) // little endian
		//      if ((x - 0x00010001) & (~x) & 0x00800080) // big endian
		if((x - 0x01010101) & (~x) & 0x80808080) { // generic
			if(p[1] == 0) {
				if (p[0] == 0 && p[2] == 1) return p;
				if (p[2] == 0 && p[3] == 1) return p + 1;
			}
			if(p[3] == 0) {
				if (p[2] == 0 && p[4] == 1) return p + 2;
				if (p[4] == 0 && p[5] == 1) return p + 3;
			}
		}
	}

	for(end += 3; p < end; p++) {
		if(p[0] == 0 && p[1] == 0 && p[2] == 1) return p;
	}

	return end + 3;
}

static const UCHAR *ff_avc_find_startcode(const UCHAR *p, const UCHAR *end){
	const UCHAR *out = ff_avc_find_startcode_internal(p, end);
	if (p<out && out<end && !out[-1]) out--;
	return out;
}

static int  GetSpsPpsFromH264Frame(UCHAR* pH264Frame, UINT32 nFrameLen)
{
	int nSpsPpsLen = 0;
	const UCHAR *NaluVcl,*NaluHeader;
	bool bfindspspps = false;
	UCHAR	*pH264FrameEnd = pH264Frame + nFrameLen;
	NaluVcl = ff_avc_find_startcode(pH264Frame,pH264FrameEnd);
	NaluHeader = NaluVcl;
	while (NaluVcl < pH264FrameEnd) {
		const UCHAR *NextNalu;
		UCHAR nal_type;
		NaluHeader = NaluVcl;
		while (!*(NaluVcl++));
		nal_type = *NaluVcl & 0x1f;
		NextNalu = ff_avc_find_startcode(NaluVcl, pH264FrameEnd);
		if (nal_type == 7 || nal_type == 8) {
			NaluVcl = NextNalu;
			NaluHeader = NaluVcl;
			bfindspspps =  true;
			continue;
		}
		break;
	}
	if(bfindspspps) nSpsPpsLen = (NaluHeader - pH264Frame);

	return nSpsPpsLen;
}

static  UCHAR*	GetNaluNoSpsPpsHeader(UCHAR* pFrameData,int& nFrameLen) //获取一帧H264数据，去掉SPS和PPS头部的信息
{
	UCHAR* pVideoData = pFrameData;
	int nSpsPpsLen = GetSpsPpsFromH264Frame(pFrameData,nFrameLen);
	if(nSpsPpsLen  > 0)
	{
		pVideoData = (pFrameData + nSpsPpsLen);
		nFrameLen -= nSpsPpsLen;
	}

	return pVideoData;
}

//----{{h264_read_seq_info----
#include "mpeg4ip_bitstream_api.h"

#ifndef false
    #define false 0
#endif
#ifndef true
    #define true 1
#endif

static void h264_decode_annexb( uint8_t *dst, int *dstlen, const uint8_t *src, const int srclen)
{
	uint8_t *dst_sav = dst;
	const uint8_t *end = &src[srclen];
    
	while(src < end)
	{
		if(src<end-3 && src[0] == 0x00 && src[1] == 0x00 && src[2] == 0x03)
		{
			*dst++ = 0x00;
			*dst++ = 0x00;
            
			src += 3;
			continue;
		}
		*dst++ = *src++;
	}
	*dstlen = dst - dst_sav;
}

static uint8_t exp_golomb_bits[256] = {
	8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,
};

uint32_t h264_ue(void *bs)
{
	uint32_t bits, read;
	int bits_left;
	uint8_t coded;
	char done = false;
	bits = 0;
	// we want to read 8 bits at a time - if we don't have 8 bits,
	// read what's left, and shift.  The exp_golomb_bits calc remains the same.
	while(done == false) {
		bits_left = C_Bitstream_bits_remain(bs);
		if (bits_left < 8) {
			read = C_Bitstream_PeekBits(bs,bits_left) << (8 - bits_left);
			done = true;
		} else {
			read = C_Bitstream_PeekBits(bs,8);
			if (read == 0) {
				C_Bitstream_GetBits(bs,8);
				bits += 8;
			} else {
				done = true;
			}
		}
	}
	coded = exp_golomb_bits[read];
	C_Bitstream_GetBits(bs, coded);
	bits += coded;
    
	//  printf("ue - bits %d\n", bits);
	return C_Bitstream_GetBits(bs,bits + 1) - 1;
}

int32_t h264_se(void *bs)
{
	uint32_t ret=0;
	int32_t temp=0;
	ret = h264_ue(bs);
	if ((ret & 0x1) == 0) {
		ret >>= 1;
		temp = 0 - ret;
		return temp;
	}
	return (ret + 1) >> 1;
}

void scaling_list (uint32_t sizeOfScalingList, void *bs)
{
	uint32_t lastScale = 8, nextScale = 8;
	uint32_t j;
    
	for (j = 0; j < sizeOfScalingList; j++) {
		if (nextScale != 0) {
			int deltaScale = h264_se(bs);
			nextScale = (lastScale + deltaScale + 256) % 256;
		}
		if (nextScale == 0) {
			lastScale = lastScale;
		} else {
			lastScale = nextScale;
		}
	}
}

int h264_read_seq_info(const uint8_t *buffer,  uint32_t buflen,  h264_decode_t *dec)
{
	uint32_t header;
	uint8_t tmp[2048]; // Should be enough for all SPS (we have at worst 13 bytes and 496 se/ue in frext)
	int tmp_len;
	void* pBSHandle=NULL;
    
	if (buffer[2] == 1) header = 4;
	else header = 5;
    
	h264_decode_annexb(tmp, &tmp_len, buffer + header, MIN(buflen-header,2048));
	pBSHandle=C_BitstreamCreate(tmp, tmp_len * 8);
	if(pBSHandle==NULL) return -3;
    
    dec->profile = C_Bitstream_GetBits(pBSHandle,8);
    C_Bitstream_GetBits(pBSHandle,1 + 1 + 1 + 1 + 4);
    dec->level = C_Bitstream_GetBits(pBSHandle,8);
    h264_ue(pBSHandle); // seq_parameter_set_id
    if (dec->profile == 100 || dec->profile == 110 ||
        dec->profile == 122 || dec->profile == 144) {
        dec->chroma_format_idc = h264_ue(pBSHandle);
        if(dec->chroma_format_idc == 3) {
            dec->residual_colour_transform_flag = C_Bitstream_GetBits(pBSHandle,1);
        }
        dec->bit_depth_luma_minus8 = h264_ue(pBSHandle);
        dec->bit_depth_chroma_minus8 = h264_ue(pBSHandle);
        dec->qpprime_y_zero_transform_bypass_flag = C_Bitstream_GetBits(pBSHandle,1);
        dec->seq_scaling_matrix_present_flag = C_Bitstream_GetBits(pBSHandle,1);
        if (dec->seq_scaling_matrix_present_flag) {
            uint32_t ix=0;
            for(ix = 0; ix < 8; ix++) {
                if(C_Bitstream_GetBits(pBSHandle,1)) scaling_list(ix < 6 ? 16 : 64, pBSHandle);
            }
        }
    }
    dec->log2_max_frame_num_minus4 = h264_ue(pBSHandle);
    dec->pic_order_cnt_type = h264_ue(pBSHandle);
    if (dec->pic_order_cnt_type == 0) {
        dec->log2_max_pic_order_cnt_lsb_minus4 = h264_ue(pBSHandle);
    } else if (dec->pic_order_cnt_type == 1) {
        uint32_t ix=0;
        dec->delta_pic_order_always_zero_flag = C_Bitstream_GetBits(pBSHandle,1);
        dec->offset_for_non_ref_pic = h264_se(pBSHandle); // offset_for_non_ref_pic
        dec->offset_for_top_to_bottom_field = h264_se(pBSHandle); // offset_for_top_to_bottom_field
        dec->pic_order_cnt_cycle_length = h264_ue(pBSHandle); // poc_cycle_length
        for(ix = 0; ix < dec->pic_order_cnt_cycle_length; ix++) {
            dec->offset_for_ref_frame[MIN(ix,255)] = h264_se(pBSHandle); // offset for ref fram -
        }
    }
    h264_ue(pBSHandle); // num_ref_frames
    C_Bitstream_GetBits(pBSHandle,1); // gaps_in_frame_num_value_allowed_flag
    
    {
        uint32_t PicHeightInMapUnits=0;
        uint32_t PicWidthInMbs = h264_ue(pBSHandle) + 1;
        dec->pic_width = PicWidthInMbs * 16;
        PicHeightInMapUnits = h264_ue(pBSHandle) + 1;
		
        dec->frame_mbs_only_flag = C_Bitstream_GetBits(pBSHandle,1);
        dec->pic_height = (2 - dec->frame_mbs_only_flag) * PicHeightInMapUnits * 16;
    }
	C_BitstreamDestroy(pBSHandle);
	return 0;
}
//----}}h264_read_seq_info----

//Desc: Get width and heigh of video from I frame data stream
//	in_pData:  the buffer of one i frame
//	nDataSize: the size of one i frame data
//	The outputed value:
//		in_out_array[0]=width
//		in_out_array[1]=heigh
int MP4::PreGetWHFromIFrame(unsigned char* in_pData, int nDataSize, int* in_out_array, int in_nArraySize)
{
	h264_decode_t h264Dec;
	int nRet=0;
	memset(&h264Dec, 0, sizeof(h264Dec));
	char bIsFrame=0;
    
	TRACE("1 PreGetWHFromIFrame, bIsFrame=%d, nDataSize=%d\n", bIsFrame, nDataSize);
	if(nDataSize<5) return -2;
	if(in_pData[0] == 0 &&  in_pData[1] == 0 && in_pData[2] == 1){
		if((in_pData[3]&0x1F)==0x07) bIsFrame=1;
	}else if(in_pData[0] == 0 &&  in_pData[1] == 0 && in_pData[2] == 0 && in_pData[3] == 1){
		if((in_pData[4]&0x1F)==0x07) bIsFrame=1;
	}else return -2;
	if(!bIsFrame) return -2;
	TRACE("2 PreGetWHFromIFrame, bIsFrame=%d, nDataSize=%d\n", bIsFrame, nDataSize);
    
	nRet=h264_read_seq_info(in_pData, nDataSize, &h264Dec);
	
	TRACE("3 PreGetWHFromIFrame, w=%d, h=%d\n", h264Dec.pic_width, h264Dec.pic_height);
	if(in_nArraySize>=2){
		in_out_array[0]=h264Dec.pic_width;
		in_out_array[1]=h264Dec.pic_height;
	}
	return nRet;
}

void my_avcodec_free_context(AVCodecContext **pavctx)
{
	AVCodecContext *avctx = *pavctx;
	if (!avctx)	return;

	avcodec_close(avctx);

	av_freep(&avctx->extradata);
	av_freep(&avctx->subtitle_header);

	av_freep(pavctx);
}

//return:
//		0: OK
//	   <0: Fail
//
int MP4::GetWHDecodeIFrame(unsigned char* in_pData, int nDataSize, int* in_out_array, int in_nArraySize)
{
	AVCodec *pCodec=NULL;
	AVCodecContext *pCodecCtx=NULL;
	AVFrame *pFrame=NULL;

	int nRet=0, got_pic = 0;
	AVPacket avpkt={0};

	pCodec = avcodec_find_decoder(AV_CODEC_ID_H264);//CODEC_ID_H264
	if(pCodec == NULL) return -1;
	
	pCodecCtx = avcodec_alloc_context3(pCodec);
	if(pCodecCtx == NULL) return -2;

	pCodecCtx->flags=0; //20170819
	if(pCodec->capabilities&AV_CODEC_CAP_TRUNCATED) pCodecCtx->flags |= AV_CODEC_FLAG_TRUNCATED;  //#define CODEC_CAP_TRUNCATED 0x0008, //#define CODEC_FLAG_TRUNCATED 0x00010000
	if(avcodec_open2(pCodecCtx, pCodec, NULL) >= 0) pFrame = av_frame_alloc(); //pFrame = avcodec_alloc_frame(); <-- ffmpeg 2.3.3
	else {
		nRet= -3;
		goto FUN_END;
	}
	if(pFrame==NULL) {
		nRet= -4;
		goto FUN_END;
	}

	av_init_packet(&avpkt);
	avpkt.data = in_pData;
	avpkt.size = nDataSize;
	nRet=avcodec_decode_video2(pCodecCtx, pFrame, &got_pic, &avpkt);
	if(got_pic<=0){
		av_init_packet(&avpkt);
		avpkt.data = in_pData;
		avpkt.size = nDataSize;
		nRet=avcodec_decode_video2(pCodecCtx, pFrame, &got_pic, &avpkt);
	}
	if(in_nArraySize>=2){
		in_out_array[0]=pFrame->width;
		in_out_array[1]=pFrame->height;
	}
FUN_END:
	if(pFrame){
		av_frame_free(&pFrame); //avcodec_free_frame(&pFrame); <-- ffmpeg 2.3.3
		pFrame = NULL;
	}
	if(pCodecCtx){
		my_avcodec_free_context(&pCodecCtx);
		pCodecCtx = NULL;
	}
	av_free_packet(&avpkt);

	if(nRet>0 && got_pic>0) return 0;
	else return nRet;
}

MP4::MP4(){
	m_oc=NULL;
	m_pffVStream=NULL;
	m_pffAStream=NULL;
	m_VStartTime=0;
	m_AStartTime=0;
	m_nVideoCodecID	=AV_CODEC_ID_H264;
	m_nSpsPpsLen=0;
	m_nLastVideoPts=-1;

	m_nASamplerate=0;
	m_ARecLastTimeMs=0;

	m_nAudioType_origin=0;
	m_pBufG711=(UCHAR *)malloc(G711_DATA_SIZES<<1);
	m_nSizBufG711=0;
	m_pBufPCM=(UCHAR *)malloc(G711_DATA_SIZES<<1);
	m_pBufAAC=(UCHAR *)malloc(AAC_ENC_SIZES);
	memset(m_pBufAAC, 0, AAC_ENC_SIZES);

	m_pEncCtx=NULL;
	m_nTimstampG711=0;

	m_bOpened=0;
	m_nFreqIdx=-1; //for aac
	pub_mutex_init(m_mutexWrite);
	av_register_all();
}

MP4::~MP4()
{
	pub_mutex_destroy(m_mutexWrite);
	if(m_pBufG711) {
		free(m_pBufG711);
		m_pBufG711=NULL;
	}
	if(m_pBufPCM) {
		free(m_pBufPCM);
		m_pBufPCM=NULL;
	}
	if(m_pBufAAC) {
		free(m_pBufAAC);
		m_pBufAAC=NULL;
	}
}

//static 
UINT32 MP4::SEMP4_GetSdkVer(char *chDesc, INT32 nDescMaxSize)
{
	if (chDesc != NULL && nDescMaxSize > 220) {
		int nVer=SE_SDK_VERSION;
		UCHAR *p=(UCHAR *)&nVer;
		sprintf(chDesc, "Mp4 lib.ver:%d.%d.%d.%d;bulid:%s;fingerprint:%s", *(p+3), *(p+2), *(p+1), *(p+0), SE_SDK_BUILD_TIME, FINGER_PRINT);
	}
	return SE_SDK_VERSION;	
}

INT32 MP4::AddVideoFrame(UCHAR *pArgFrameBuffer,INT32 iSrcPos, INT32 nFrameLen, UINT32 nTimstamp, INT32 bKeyFrame)
{
	INT32	nRet=0;
	AVDictionary *options = NULL;
	uint8_t *pData = NULL;
	UCHAR *pFrameBuffer=pArgFrameBuffer+iSrcPos;

pub_mutex_lock(m_mutexWrite);
	if(!m_bOpened){
		pub_mutex_unlock(m_mutexWrite);
		return -6;
	}
	if(m_nVideoCodecID==AV_CODEC_ID_MJPEG){
		if(m_oc->start_time == -1)
		{
			nRet=avio_open(&m_oc->pb, m_oc->filename, AVIO_FLAG_READ_WRITE);
			if(nRet < 0)
			{
				TRACE("AddVideoFrame] mjpeg avio_open error=%d m_nVideoCodecID=%d\n",nRet, m_nVideoCodecID);
				nRet=-1;
				goto FUN_END;
			}
			//pData =  (uint8_t *)av_mallocz(nSpsPpsLen);
			m_pffVStream->codec->extradata      = NULL;
			//memcpy(m_pffVStream->codec->extradata, pFrameBuffer, nSpsPpsLen);
			m_pffVStream->codec->extradata_size= 0;//nSpsPpsLen;
	
			//nRet = av_dict_set(&options, "movflags", "rtphint", 1);
			if((nRet = avformat_write_header(m_oc, NULL)) != 0)//&options
			{
				TRACE("AddVideoFrame] mjpeg avformat_write_header error,%d \n",nRet);
				nRet=-2;
				goto FUN_END;
			}
			m_oc->start_time = 0;
			m_pffVStream->start_time = 0;
			m_VStartTime = nTimstamp;
		}
	}else{ //H264,H265 stream
		//TRACE("H264 m_oc=%p bKeyFrame=%d m_oc->start_time=%d\n", m_oc, bKeyFrame, m_oc->start_time);
		if(m_oc->start_time == -1){
			int  arrWH[2]={0,0};
			int  nSpsPpsLen= 0, nR=0;
			UCHAR chSpsPps[800]={0};
			memset(chSpsPps, 0, sizeof(chSpsPps));

			if(AV_CODEC_ID_H264==m_nVideoCodecID){
				nSpsPpsLen = GetSpsPpsFromH264Frame(pFrameBuffer,nFrameLen); //生成sps和pps
				if(nSpsPpsLen>0) memcpy(chSpsPps, pFrameBuffer, nSpsPpsLen);
				TRACE("GetSpsPpsFromH264Frame=%d\n", nSpsPpsLen);

			}else if(AV_CODEC_ID_H265==m_nVideoCodecID){
				nSpsPpsLen=h265GetVpsSpsPps(pFrameBuffer, nFrameLen, bKeyFrame, chSpsPps);
			}

			if(nSpsPpsLen <= 0){
				TRACE("AddVideoFrame] GetSpsPpsFromH264Frame error,%d, m_nVideoCodecID=%d\n",nSpsPpsLen, m_nVideoCodecID);
				nRet=-1;
				goto FUN_END;
			}
			if(avio_open(&m_oc->pb, m_oc->filename, AVIO_FLAG_READ_WRITE) !=  0){
				TRACE("AddVideoFrame] avio_open error,%d \n",nRet);
				nRet=-2;
				goto FUN_END;
			}
			m_nSpsPpsLen=nSpsPpsLen;
			pData = (uint8_t *)av_mallocz(nSpsPpsLen);
			m_pffVStream->codec->extradata      = pData;
			memcpy(m_pffVStream->codec->extradata, chSpsPps, nSpsPpsLen);
			m_pffVStream->codec->extradata_size= nSpsPpsLen;
			
			//get width and heigh from I frame
			if(AV_CODEC_ID_H264==m_nVideoCodecID && (m_pffVStream->codec->width==0 || m_pffVStream->codec->height==0)){ //20210808: added:width,height==0的判断
				//PreGetWHFromIFrame(pFrameBuffer,nFrameLen, arrWH, 2); //-->640*368
				nR=GetWHDecodeIFrame(pFrameBuffer,nFrameLen, arrWH, 2);	//-->640*360

			}else if(AV_CODEC_ID_H265==m_nVideoCodecID){
				vc_params_t params = { 0 };
				memset(chSpsPps, 0, sizeof(chSpsPps));
				nR=h265GetSps(pFrameBuffer, nFrameLen, bKeyFrame, chSpsPps);
				nR=h265ParseSeqParaSet(&chSpsPps[4], nR-4, &params);
				arrWH[0]=params.width;
				arrWH[1]=params.height;
			}
			TRACE("AddVideoFrame] WxH=%dx%d \n", arrWH[0], arrWH[1]);
			if(m_pffVStream->codec->width == 0 || m_pffVStream->codec->height == 0) { //20210808: added:width,height==0的判断
				m_pffVStream->codec->width = arrWH[0];
				m_pffVStream->codec->height = arrWH[1];
			}

			//nRet = av_dict_set(&options, "movflags", "rtphint", 1);
			//if((nRet = avformat_write_header(m_oc, &options)) != 0)
            if((nRet = avformat_write_header(m_oc, NULL)) != 0)
			{
				TRACE("AddVideoFrame] avformat_write_header error,%d \n",nRet);
				nRet=-3;
				goto FUN_END;
			}
			m_oc->start_time = 0;
			m_pffVStream->start_time = 0;
			m_VStartTime = nTimstamp;
		}
		if(bKeyFrame==1) {
			int nStripLen = 0;
			if(AV_CODEC_ID_H264==m_nVideoCodecID){
				nStripLen = GetSpsPpsFromH264Frame(pFrameBuffer, nFrameLen);
			}else if(AV_CODEC_ID_H265==m_nVideoCodecID){
				UCHAR chSpsPps[800]={0};
				nStripLen = h265GetVpsSpsPps(pFrameBuffer, nFrameLen, bKeyFrame, chSpsPps);
			}
			if(nStripLen > 0) {
				pFrameBuffer += nStripLen;
				nFrameLen -= nStripLen;
			}
		}
		m_nVideoFrmNum++;
	}

	{
		AVPacket Pkt = { 0 };
		av_init_packet(&Pkt);
		Pkt.pts = (int64_t)(nTimstamp - m_VStartTime);
		if(Pkt.pts == 0) Pkt.pts = 0x01;
		//m_FileRecordedSecond = Pkt.pts/1000;
		Pkt.pts *= TIME_ST_FREQ;
		if(m_nLastVideoPts >= 0 && Pkt.pts <= m_nLastVideoPts) {
			Pkt.pts = m_nLastVideoPts + (TIME_BASE_AV / 20);
		}
		m_nLastVideoPts = Pkt.pts;
		Pkt.dts  = Pkt.pts;
		Pkt.data = (uint8_t*)pFrameBuffer;
		Pkt.size = nFrameLen;
		Pkt.stream_index = m_pffVStream->index;
		Pkt.flags |= bKeyFrame?AV_PKT_FLAG_KEY:0;
		nRet = av_interleaved_write_frame(m_oc, &Pkt); //may return -22;

		//return AVERROR(EINVAL); //-22
		//if(pkt->dts == AV_NOPTS_VALUE && !(s->oformat->flags & AVFMT_NOTIMESTAMPS))

		//TRACE("AddVideoFrame] av_interleaved_write_frame nRet=%d  m_oc->start_time=%d\n",nRet, m_oc->start_time);
		//if(nRet != 0) nRet = RETERR(ERR_WRITEFRAME);
		av_free_packet(&Pkt);
	}

FUN_END:
	if(nRet != 0){
		if(pData) {
			av_freep(pData);
			pData=NULL;
		}
	}
pub_mutex_unlock(m_mutexWrite);
	return nRet;
}

//return:
//	-12: g711数据没有达到1K
//	-13: pcm数据不够，还不能编码一帧aac
//
int MP4::inner_encG711AtoAAC(UCHAR *pDataG711, int nBytesG711, UCHAR *pBufG711, int *pnSizBufG711,UCHAR *pBufPCM, UCHAR *pBufAAC){
	int nSiz=0;

	if(nBytesG711 > 0) {
		memcpy(pBufG711 + *pnSizBufG711, pDataG711, nBytesG711);
		*pnSizBufG711 += nBytesG711;
	}
	if(*pnSizBufG711 < G711_DATA_SIZES) {
		return -12;
	}

	int nLenAfterEnc=AAC_ENC_SIZES;
	//g711 --> pcm, get 2K pcm data
	decG711toPCM(pBufG711, G711_DATA_SIZES, pBufPCM); //pBufG711, pBufPCM: 2K;
	//g711-->pcm的g711数据移除, pBufG711余下的数据移到buf头
	memcpy(pBufG711, pBufG711+G711_DATA_SIZES,  *pnSizBufG711-G711_DATA_SIZES);
	*pnSizBufG711 -= G711_DATA_SIZES; //减去本次解码的g711数据

	nSiz = aacEnc_frame_with_adts_head(m_pEncCtx,pBufPCM, AAC_ENC_SIZES<<1, pBufAAC, &nLenAfterEnc); //pBufAAC: 1K
	if(nSiz >0) {
		nSiz=nLenAfterEnc;
	}else{
		printf("    aacEnc_frame(.)=%d nLenAfterEnc=%d \n", nSiz, nLenAfterEnc);
		nSiz=-13;
	}
	return nSiz;
}

//return:
//	-100: data is too smaller

int MP4::FlushG711Remainder(){
	if(m_nSizBufG711 <= 0 || m_pEncCtx == NULL || m_pffAStream == NULL) return 0;
	if(m_oc == NULL || m_oc->start_time < (int64_t)0) return 0;

	while(m_nSizBufG711 > 0 && m_nSizBufG711 < G711_DATA_SIZES){
		m_pBufG711[m_nSizBufG711++] = (UCHAR)0xD5; // g711a silence
	}
	if(m_nSizBufG711 < G711_DATA_SIZES) return 0;

	int nLenAfterEnc = AAC_ENC_SIZES;
	decG711toPCM(m_pBufG711, G711_DATA_SIZES, m_pBufPCM);
	int nAacLen = aacEnc_frame_with_adts_head(m_pEncCtx, m_pBufPCM, AAC_ENC_SIZES<<1, m_pBufAAC, &nLenAfterEnc);
	m_nSizBufG711 = 0;
	if(nAacLen <= 0) return -100;

	UINT32 nTimstamp = m_ARecLastTimeMs >= m_VStartTime ? m_ARecLastTimeMs : m_VStartTime;
	int64_t nPackTimeStamp = (int64_t)nTimstamp - (int64_t)m_VStartTime;
	if(nPackTimeStamp < 0) nPackTimeStamp = 0;
	int64_t nFramePts = nPackTimeStamp * (m_nASamplerate / 1000);
	if(nFramePts <= (int64_t)0) nFramePts = 0x01;

	AVPacket Pkt = { 0 };
	av_init_packet(&Pkt);
	Pkt.dts = Pkt.pts = nFramePts;
	Pkt.data = (uint8_t*)(m_pBufAAC + 7);
	Pkt.size = nLenAfterEnc - 7;
	Pkt.stream_index = m_pffAStream->index;
	Pkt.flags |= AV_PKT_FLAG_KEY;

	int ret = av_interleaved_write_frame(m_oc, &Pkt);
	av_free_packet(&Pkt);
	if(ret == 0 && m_ARecLastTimeMs < nTimstamp) m_ARecLastTimeMs = nTimstamp;
	return ret;
}

//g711-->aac, then write mp4
int MP4::AddAudioFrame_g711(UCHAR *pFrameBuffer,INT32 iSrcPos,int nFrameLen, UINT32 nTimstamp){
	pub_mutex_lock(m_mutexWrite);
	if(!m_bOpened) {
		pub_mutex_unlock(m_mutexWrite);
		return -6;
	}
	if(m_oc == NULL || m_oc->start_time < (int64_t)0 || m_pffAStream == NULL) {
		pub_mutex_unlock(m_mutexWrite);
		return -4;
	}
	if((int64_t)nTimstamp < (int64_t)m_VStartTime) {
		pub_mutex_unlock(m_mutexWrite);
		return -3;
	}

	int ret = 0;
	int retLast = 1;
	int nWriteCount = 0;

	while(1){
		ret = inner_encG711AtoAAC(pFrameBuffer + iSrcPos, nFrameLen, m_pBufG711, &m_nSizBufG711, m_pBufPCM, m_pBufAAC);
		nFrameLen = 0; // only feed source data once
		if(ret == -12) break;
		if(ret <= 0){
			retLast = -100;
			break;
		}

		AVPacket Pkt = { 0 };
		av_init_packet(&Pkt);

		int64_t nPackTimeStamp = (int64_t)nTimstamp - (int64_t)m_VStartTime;
		if(m_ARecLastTimeMs >= nTimstamp){
			nPackTimeStamp = (int64_t)m_ARecLastTimeMs - (int64_t)m_VStartTime + 1;
		}
		if(nPackTimeStamp < 0) nPackTimeStamp = 0;

		int64_t nFramePts = nPackTimeStamp * (m_nASamplerate / 1000);
		if(nFramePts <= (int64_t)0) nFramePts = 0x01;
		Pkt.dts = Pkt.pts = nFramePts;

		Pkt.data = (uint8_t*)(m_pBufAAC + 7);
		Pkt.size = ret - 7;
		Pkt.stream_index = m_pffAStream->index;
		Pkt.flags |= AV_PKT_FLAG_KEY;

		ret = av_interleaved_write_frame(m_oc, &Pkt);
		av_free_packet(&Pkt);
		if(ret != 0) {
			retLast = ret;
			break;
		}
		if(m_ARecLastTimeMs < nTimstamp) m_ARecLastTimeMs = nTimstamp;
		nWriteCount++;
		retLast = 0;
	}
pub_mutex_unlock(m_mutexWrite);
	if(nWriteCount > 0) return 0;
	return retLast;
}

//return:
//	  -2 ~ -5 
//	  -6: !m_bOpened
//	-100: failed to inner_encG711AtoAAC
//
int MP4::AddAudioFrame(UCHAR *pFrameBuffer,INT32 iSrcPos,int nFrameLen, UINT32 nTimstamp)
{
	if(RTSP_AV_CODECID_AUDIO_G711A==m_nAudioType_origin){
		return AddAudioFrame_g711(pFrameBuffer,iSrcPos, nFrameLen, nTimstamp);
	}
	INT32 nRet=0;
	if(m_oc==NULL) return -2;
	if(nTimstamp < m_VStartTime) return -3;
	if(m_oc->start_time < (int64_t)0) return -4;
	if(m_pffAStream==NULL) return -5;

pub_mutex_lock(m_mutexWrite);
	if(!m_bOpened){
		pub_mutex_unlock(m_mutexWrite);
		return -6;
	}
	AVPacket Pkt = { 0 };
	int64_t  nFramePts = 0;
	UINT32	 nPackTimeStamp = nTimstamp - m_VStartTime;

	av_init_packet(&Pkt);
	if(m_ARecLastTimeMs  >=  nTimstamp){
		nPackTimeStamp = m_ARecLastTimeMs - m_VStartTime;
		nPackTimeStamp+=1;
	}
	if(m_pffAStream->start_time == (int64_t)-1LL) m_pffAStream->start_time = 0;

	nFramePts = (int64_t)nPackTimeStamp;
	//Pkt.pts = (int64_t)(pAudioHeader->timstamp - m_VStartTime);
	nFramePts *=  (m_nASamplerate/ 1000);
	if(nFramePts <= (int64_t)0) nFramePts = 0x01;
	Pkt.dts = Pkt.pts = nFramePts;

	AVCodecContext *pACodec	 = m_pffAStream->codec;
	if(pACodec->codec_id==AV_CODEC_ID_AAC){
			Pkt.data = (uint8_t*)(pFrameBuffer+7+iSrcPos);
			Pkt.size = nFrameLen-7;
	}else{
		//juju
		Pkt.data = (uint8_t*)(pFrameBuffer+iSrcPos);
		Pkt.size = nFrameLen;
	}
	Pkt.stream_index = m_pffAStream->index;
	Pkt.flags |= AV_PKT_FLAG_KEY;	

	nRet = av_interleaved_write_frame(m_oc, &Pkt);
	if(nRet != 0){
		char errbuf[128] = {0};
		av_strerror(nRet, errbuf, sizeof(errbuf));
		//printf("RecordAudioFrame av_interleaved_write_frame :%d,%s \n",nRet,errbuf);
	}
	m_ARecLastTimeMs = nTimstamp;
	av_free_packet(&Pkt);
FUN_END:	
pub_mutex_unlock(m_mutexWrite);

	return nRet;
}

INT32 MP4::AddVideoFrame_ts(UCHAR *pArgFrameBuffer,INT32 iSrcPos, INT32 nFrameLen, UINT32 nTimstamp, INT32 bKeyFrame)
{
	INT32	nRet=0;
	AVDictionary *options = NULL;
	uint8_t *pData = NULL;
	UCHAR *pFrameBuffer=pArgFrameBuffer+iSrcPos;

	pub_mutex_lock(m_mutexWrite);
	if(!m_bOpened){
		pub_mutex_unlock(m_mutexWrite);
		return -6;
	}
	if(m_nVideoCodecID==AV_CODEC_ID_MJPEG){
		if(m_oc->start_time == -1)
		{
			nRet=avio_open(&m_oc->pb, m_oc->filename, AVIO_FLAG_READ_WRITE);
			if(nRet < 0)
			{
				TRACE("AddVideoFrame] mjpeg avio_open error=%d m_nVideoCodecID=%d\n",nRet, m_nVideoCodecID);
				nRet=-1;
				goto FUN_END;
			}
			//pData =  (uint8_t *)av_mallocz(nSpsPpsLen);
			m_pffVStream->codec->extradata      = NULL;
			//memcpy(m_pffVStream->codec->extradata, pFrameBuffer, nSpsPpsLen);
			m_pffVStream->codec->extradata_size= 0;//nSpsPpsLen;

			//nRet = av_dict_set(&options, "movflags", "rtphint", 1);
			if((nRet = avformat_write_header(m_oc, NULL)) != 0)//&options
			{
				TRACE("AddVideoFrame] mjpeg avformat_write_header error,%d \n",nRet);
				nRet=-2;
				goto FUN_END;
			}
			m_oc->start_time = 0;
			m_pffVStream->start_time = 0;
			m_VStartTime = nTimstamp;
		}
	}else{ //H264,H265 stream
		//TRACE("H264 m_oc=%p bKeyFrame=%d m_oc->start_time=%d\n", m_oc, bKeyFrame, m_oc->start_time);
		if(m_oc->start_time == -1){
			int  arrWH[2]={0,0};
			int  nSpsPpsLen= 0, nR=0;
			UCHAR chSpsPps[800]={0};
			memset(chSpsPps, 0, sizeof(chSpsPps));

			if(AV_CODEC_ID_H264==m_nVideoCodecID){
				nSpsPpsLen = GetSpsPpsFromH264Frame(pFrameBuffer,nFrameLen); //生成sps和pps
				if(nSpsPpsLen>0) memcpy(chSpsPps, pFrameBuffer, nSpsPpsLen);
				TRACE("GetSpsPpsFromH264Frame=%d\n", nSpsPpsLen);

			}else if(AV_CODEC_ID_H265==m_nVideoCodecID){
				nSpsPpsLen=h265GetVpsSpsPps(pFrameBuffer, nFrameLen, bKeyFrame, chSpsPps);
			}

			if(nSpsPpsLen <= 0){
				TRACE("AddVideoFrame] GetSpsPpsFromH264Frame error,%d, m_nVideoCodecID=%d\n",nSpsPpsLen, m_nVideoCodecID);
				nRet=-1;
				goto FUN_END;
			}
			if(avio_open(&m_oc->pb, m_oc->filename, AVIO_FLAG_READ_WRITE) !=  0){
				TRACE("AddVideoFrame] avio_open error,%d \n",nRet);
				nRet=-2;
				goto FUN_END;
			}
			m_nSpsPpsLen=nSpsPpsLen;
			pData = (uint8_t *)av_mallocz(nSpsPpsLen);
			m_pffVStream->codec->extradata      = pData;
			memcpy(m_pffVStream->codec->extradata, chSpsPps, nSpsPpsLen);
			m_pffVStream->codec->extradata_size= nSpsPpsLen;

			//get width and heigh from I frame
			if(AV_CODEC_ID_H264==m_nVideoCodecID){
				//PreGetWHFromIFrame(pFrameBuffer,nFrameLen, arrWH, 2); //-->640*368
				nR=GetWHDecodeIFrame(pFrameBuffer,nFrameLen, arrWH, 2);	//-->640*360

			}else if(AV_CODEC_ID_H265==m_nVideoCodecID){
				vc_params_t params = { 0 };
				memset(chSpsPps, 0, sizeof(chSpsPps));
				nR=h265GetSps(pFrameBuffer, nFrameLen, bKeyFrame, chSpsPps);
				nR=h265ParseSeqParaSet(&chSpsPps[4], nR-4, &params);
				arrWH[0]=params.width;
				arrWH[1]=params.height;
			}
			TRACE("AddVideoFrame] WxH=%dx%d \n", arrWH[0], arrWH[1]);
			m_pffVStream->codec->width =arrWH[0];
			m_pffVStream->codec->height=arrWH[1];

			//nRet = av_dict_set(&options, "movflags", "rtphint", 1);
			//if((nRet = avformat_write_header(m_oc, &options)) != 0)
			if((nRet = avformat_write_header(m_oc, NULL)) != 0)
			{
				TRACE("AddVideoFrame] avformat_write_header error,%d \n",nRet);
				nRet=-3;
				goto FUN_END;
			}
			m_oc->start_time = 0;
			m_pffVStream->start_time = 0;
			m_VStartTime = nTimstamp;
		}
		if(bKeyFrame==1) {
			int nStripLen = 0;
			if(AV_CODEC_ID_H264==m_nVideoCodecID){
				nStripLen = GetSpsPpsFromH264Frame(pFrameBuffer, nFrameLen);
			}else if(AV_CODEC_ID_H265==m_nVideoCodecID){
				UCHAR chSpsPps[800]={0};
				nStripLen = h265GetVpsSpsPps(pFrameBuffer, nFrameLen, bKeyFrame, chSpsPps);
			}
			if(nStripLen > 0) {
				pFrameBuffer += nStripLen;
				nFrameLen -= nStripLen;
			}
		}
		m_nVideoFrmNum++;
	}

	{
		AVPacket Pkt = { 0 };
		av_init_packet(&Pkt);
		Pkt.pts = (int64_t)(nTimstamp - m_VStartTime);
		if(Pkt.pts == 0) Pkt.pts = 0x01;
		//m_FileRecordedSecond = Pkt.pts/1000;
		Pkt.pts *= TIME_ST_FREQ;
		if(m_nLastVideoPts >= 0 && Pkt.pts <= m_nLastVideoPts) {
			Pkt.pts = m_nLastVideoPts + (TIME_BASE_AV / 20);
		}
		m_nLastVideoPts = Pkt.pts;
		Pkt.dts  = Pkt.pts;
		Pkt.data = (uint8_t*)pFrameBuffer;
		Pkt.size = nFrameLen;
		Pkt.stream_index = m_pffVStream->index;
		Pkt.flags |= bKeyFrame?AV_PKT_FLAG_KEY:0;
		nRet = av_interleaved_write_frame(m_oc, &Pkt); //may return -22;

		//return AVERROR(EINVAL); //-22
		//if(pkt->dts == AV_NOPTS_VALUE && !(s->oformat->flags & AVFMT_NOTIMESTAMPS))

		//TRACE("AddVideoFrame] av_interleaved_write_frame nRet=%d  m_oc->start_time=%d\n",nRet, m_oc->start_time);
		//if(nRet != 0) nRet = RETERR(ERR_WRITEFRAME);
		av_free_packet(&Pkt);
	}

FUN_END:
	if(nRet != 0){
		if(pData) {
			av_freep(pData);
			pData=NULL;
		}
	}
	pub_mutex_unlock(m_mutexWrite);
	return nRet;
}
//return:
//	<0: fail
//	-2, -4, -5, -6, 
int MP4::AddAudioFrame_ts(UCHAR *pFrameBuffer,INT32 iSrcPos,int nFrameLen, UINT32 nTimstamp)
{
	INT32 nRet=0;

	if(m_oc==NULL) return -2;
	if(m_oc->start_time < (int64_t)0) return -4;
	if(m_pffAStream==NULL) return -5;

	pub_mutex_lock(m_mutexWrite);
	if(!m_bOpened){
		pub_mutex_unlock(m_mutexWrite);
		return -6;
	}
	AVPacket Pkt = { 0 };
	int64_t  nFramePts = 0LL;
	INT32	 nPackTimeStamp = 0;//nTimstamp - m_VStartTime;
	if(m_ARecLastTimeMs==0) m_ARecLastTimeMs=nTimstamp;

	av_init_packet(&Pkt);
	nPackTimeStamp=nTimstamp-m_ARecLastTimeMs;
	if(nPackTimeStamp<0) 
		nPackTimeStamp=m_ARecLastTimeMs+1; 
	if(m_pffAStream->start_time == (int64_t)-1LL) m_pffAStream->start_time = 0;

	nFramePts = (int64_t)nPackTimeStamp;
	nFramePts *=  (m_nASamplerate/ 1000);
	if(nFramePts < (int64_t)0LL) nFramePts = 0x01;
	Pkt.dts = Pkt.pts = nFramePts;

	AVCodecContext *pACodec	 = m_pffAStream->codec;
	if(pACodec->codec_id==AV_CODEC_ID_AAC){
		Pkt.data = (uint8_t*)(pFrameBuffer+7+iSrcPos);
		Pkt.size = nFrameLen-7;
	}else{
		Pkt.data = (uint8_t*)(pFrameBuffer+iSrcPos);
		Pkt.size = nFrameLen;
	}
	Pkt.stream_index = m_pffAStream->index;
	Pkt.flags |= AV_PKT_FLAG_KEY;	

	nRet = av_interleaved_write_frame(m_oc, &Pkt);
	if(nRet != 0){
		char errbuf[128] = {0};
		av_strerror(nRet, errbuf, sizeof(errbuf));
		//printf("RecordAudioFrame av_interleaved_write_frame :%d,%s \n",nRet,errbuf);
	}
	//m_ARecLastTimeMs = nTimstamp;

	av_free_packet(&Pkt);
	pub_mutex_unlock(m_mutexWrite);

	return nRet;
}

int MP4::OpenMp4File(char *pFilename, int nVideoWidth, int nVideoHeigh, int nVideoType, INT32 nAudioSampleRate, INT32 nAudioChannel, INT32 nAudioType, INT32 nRotateVideo)
{
	if(pFilename==NULL || nVideoWidth<0 || nVideoHeigh<0) return -1;
	if(nVideoType<=RTSP_AV_CODECID_UNKNOWN || nVideoType>RTSP_AV_CODECID_AUDIO_AAC) return -2;

#ifdef LINUX_ANDROID
	//if(!IsLegalAPIforMP4()) return -4; //removed on v1.0.4.4
#endif

	if(nVideoType==RTSP_AV_CODECID_VIDEO_MJPEG) m_nVideoCodecID=AV_CODEC_ID_MJPEG;
	else {
		switch(nVideoType){
			default:
			case RTSP_AV_CODECID_VIDEO_H264:
				m_nVideoCodecID=AV_CODEC_ID_H264;
				break;
			case RTSP_AV_CODECID_VIDEO_H265:
				m_nVideoCodecID=AV_CODEC_ID_H265;
				break;
		}
	}

	INT32 nRet=0;
	char bUseMovContainer =0;
	AVOutputFormat	*ofmt = NULL;
	AVCodec			*pavcode = avcodec_find_encoder((AVCodecID)m_nVideoCodecID);
	AVCodecContext	*pAVCodec=NULL;
	//AVCodecParameters *pVCodecpar=NULL, *pACodecpar=NULL; //for ffmpeg 3.2

	if(m_oc){
		avio_close(m_oc->pb);
		if(m_oc->filename) remove(m_oc->filename);
		for(int i = 0; i<m_oc->nb_streams; i++){
			if(m_oc->streams[i]->codec)	avcodec_close(m_oc->streams[i]->codec);
		}
		avformat_free_context(m_oc);
		m_oc = 0;
		m_pffVStream = 0;
		m_pffAStream = 0;
	}

	m_oc=avformat_alloc_context();
	strcpy(m_oc->filename, pFilename);
// 	if(avformat_alloc_output_context2(&m_oc, NULL, NULL, pFilename) < 0){
// 		TRACE("OpenMp4File] Mp4Muxer alloc output context2 failed \n");
// 		return -3;
// 	}

	m_nAudioType_origin=nAudioType;

	#ifdef AUDIO_G711_2_AAC_WHEN_REC_MP4
		if(RTSP_AV_CODECID_AUDIO_G711A==nAudioType) nAudioType=RTSP_AV_CODECID_AUDIO_AAC;
	#endif
	// Container follows output filename suffix:
	// ".mov" -> mov, otherwise default to mp4.
	bUseMovContainer = ends_with_mov_ext(pFilename);

	if(bUseMovContainer) ofmt = av_guess_format("mov", NULL, NULL);
	else ofmt = av_guess_format("mp4", NULL, NULL);
	m_oc->oformat = ofmt;
	m_oc->oformat->video_codec=(AVCodecID)m_nVideoCodecID;
	m_oc->oformat->audio_codec=AV_CODEC_ID_NONE;

	m_pffVStream				= avformat_new_stream(m_oc, pavcode);

	//pVCodecpar=m_pffVStream->codecpar; //juju2018
//printf("OpenMp4File] avcodec_find_encoder(%d)=0x%X, m_pffVStream=0x%X\n", m_nVideoCodecID, pavcode, m_pffVStream);
	pAVCodec					= m_pffVStream->codec;
	m_oc->start_time			= -1LL;
	pAVCodec->codec_id			= (AVCodecID)m_nVideoCodecID;
	if(m_nVideoCodecID==AV_CODEC_ID_H265){
		// Force 'hvc1' sample entry for better HEVC compatibility on some Android vendors.
		pAVCodec->codec_tag = ((unsigned int)'h') | ((unsigned int)'v'<<8) | ((unsigned int)'c'<<16) | ((unsigned int)'1'<<24);
	}
	pAVCodec->width				= nVideoWidth;  //to MJPeg, don't set for h264
	pAVCodec->height			= nVideoHeigh;	//to MJPeg, don't set for h264
	pAVCodec->codec_type		= AVMEDIA_TYPE_VIDEO;
	pAVCodec->time_base.den		= TIME_BASE_AV;
	pAVCodec->time_base.num		= 1;
	pAVCodec->has_b_frames		= 0;//juju
	m_pffVStream->time_base.den	= TIME_BASE_AV;
	m_pffVStream->time_base.num	= 1;
	if(m_oc->oformat->flags & AVFMT_GLOBALHEADER) pAVCodec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	
	if(nRotateVideo>0 && nRotateVideo<360){
		CHAR cTmp[8]={0};
		sprintf(cTmp, "%d", nRotateVideo);
		nRet = av_dict_set(&m_pffVStream->metadata,"rotate",cTmp,0); //set rotate 90 du
		if(nRet>=0){ } //ok
	}
	//avcodec_parameters_from_context(pVCodecpar, pAVCodec); //juju2018

//to do audio
	m_ARecLastTimeMs=0;
	if(nAudioType > 0)
	{
		int					rate_index;
		UCHAR				audioObjectType = 2;
		enum AVCodecID		eCodecID = (AVCodecID)0; //(enum AVCodecID)nAudioType;
		UCHAR				audioSpecificConfig[2];
		if(nAudioType==RTSP_AV_CODECID_AUDIO_AAC) eCodecID=(AVCodecID)AV_CODEC_ID_AAC;
		else if(nAudioType==RTSP_AV_CODECID_AUDIO_G726)  eCodecID =(AVCodecID)AV_CODEC_ID_ADPCM_G726;
		else if(nAudioType==RTSP_AV_CODECID_AUDIO_G711A) eCodecID =(AVCodecID)AV_CODEC_ID_PCM_ALAW;
		else if(nAudioType==RTSP_AV_CODECID_AUDIO_ADPCM) eCodecID =(AVCodecID)AV_CODEC_ID_ADPCM_IMA_QT;
		else {
			m_oc->oformat->audio_codec=AV_CODEC_ID_NONE;
			nRet=-3;
			goto FUN_OVER;
		}
		m_oc->oformat->audio_codec=eCodecID;

		pavcode = avcodec_find_encoder(eCodecID);
		m_pffAStream = avformat_new_stream(m_oc, pavcode);
		//pACodecpar=m_pffAStream->codecpar; //juju2018

		pAVCodec	 = m_pffAStream->codec;
		pAVCodec->codec_id			= eCodecID;
		pAVCodec->codec_type		= AVMEDIA_TYPE_AUDIO;
		pAVCodec->time_base.den		= nAudioSampleRate;
		pAVCodec->time_base.num		= 1;
		pAVCodec->sample_rate = nAudioSampleRate;
		pAVCodec->channels	  = nAudioChannel;
		if(nAudioChannel==1) pAVCodec->channel_layout=AV_CH_LAYOUT_MONO; //2:left, 3:left,right
		else if(nAudioChannel==2) pAVCodec->channel_layout=AV_CH_LAYOUT_STEREO; //2:left, 3:left,right
		pAVCodec->frame_size = AUDIO_FRAME_SIZE;

		m_pffAStream->time_base.den	= nAudioSampleRate;
		m_pffAStream->time_base.num	= 1;
		m_pffAStream->start_time    = -1;
		if(m_oc->oformat->flags & AVFMT_GLOBALHEADER) pAVCodec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

		m_nASamplerate=nAudioSampleRate;
		if(eCodecID==AV_CODEC_ID_AAC){
			for(rate_index = 0; rate_index < 16; rate_index++){ //for AAC
				if(g_AudioSampleRates[rate_index] == nAudioSampleRate) break;
			}
			//if(rate_index == 16) return RETERR(ERR_AUDIORATE);
			if(rate_index<16){
				audioSpecificConfig[0] = (audioObjectType<<3) | (rate_index>>1);
				audioSpecificConfig[1] = (rate_index<<7) | (nAudioChannel<<3);
				m_pffAStream->codec->extradata      = (uint8_t *)av_mallocz(2);
				memcpy(m_pffAStream->codec->extradata, audioSpecificConfig, 2);
				m_pffAStream->codec->extradata_size= 2;
			}
		}
		//avcodec_parameters_from_context(pACodecpar, pAVCodec); //juju2018
		m_pEncCtx=aacEnc_init(nAudioSampleRate,1,16, nAudioSampleRate*2);
	}
FUN_OVER:
	m_nSpsPpsLen=0;
	if(nRet<0){
		if(m_oc){
			avio_close(m_oc->pb);
			if(m_oc->filename) remove(m_oc->filename);
			for(int i = 0; i<m_oc->nb_streams; i++){
				if(m_oc->streams[i]->codec)	avcodec_close(m_oc->streams[i]->codec);
			}
			avformat_free_context(m_oc);
			m_oc = 0;
			m_pffVStream = 0;
			m_pffAStream = 0;
		}
	}else {
		pub_mutex_lock(m_mutexWrite);
			m_bOpened=1;
			m_nVideoFrmNum=0;
			m_nLastVideoPts=-1;
		pub_mutex_unlock(m_mutexWrite);
	}

	return nRet;
}

//return :
//	>=0 OK
//	 <0 fail
//
int MP4::CloseMp4File()
{
	int nRet = 0;
	CHAR chFilenameTmp[400]={0};
	pub_mutex_lock(m_mutexWrite);
	if(m_nAudioType_origin == RTSP_AV_CODECID_AUDIO_G711A && m_oc && m_oc->start_time >= 0) {
		int nFlush = FlushG711Remainder();
		TRACE("CloseMp4File] FlushG711Remainder=%d remainBuf=%d\n", nFlush, m_nSizBufG711);
	}
	m_bOpened=0;
	pub_mutex_unlock(m_mutexWrite);

	if(!m_oc) return nRet;
	if(m_oc->start_time <0)
	{
		TRACE("CloseMp4File] \n");
		avio_close(m_oc->pb);
		if(m_oc->filename[0]!='\0') remove(m_oc->filename);
		for(int i = 0; i<m_oc->nb_streams; i++){
			if(m_oc->streams[i]->codec)	avcodec_close(m_oc->streams[i]->codec);
		}
		avformat_free_context(m_oc);
		m_oc = 0;
		m_pffVStream = 0;
		return nRet;
	}

	nRet = av_write_trailer(m_oc); //May only be called after a successful call to avformat_write_header.

	if(nRet !=  0){
		TRACE("CloseMp4File] av_write_trailer failed,nRet = %d \n",nRet);
		nRet = -1;
	}

	strcpy(chFilenameTmp, m_oc->filename);
	nRet = avio_close(m_oc->pb);
	if(nRet != 0){
		TRACE("CloseMp4File] avio_close failed,nRet = %d\n",nRet);
		nRet = -2;
	}
	for(int i = 0; i<m_oc->nb_streams; i++){
		if(m_oc->streams[i]->codec)	avcodec_close(m_oc->streams[i]->codec);
	}
	avformat_free_context(m_oc);
	
	m_oc = 0;
	m_pffVStream = 0;
	m_pffAStream = 0;
	
	// Fallback compatibility patch:
	// if some muxing path still writes HEVC sample entry as "hev1",
	// rewrite it to "hvc1" for better Android/Samsung compatibility.
	if(nRet == 0 && m_nVideoCodecID == AV_CODEC_ID_H265 && !ends_with_mov_ext(chFilenameTmp)){
		int nPatch = patch_hev1_to_hvc1_in_file(chFilenameTmp);
		TRACE("h265 CloseMp4File fallback patch: %d, file=%s\n", nPatch, chFilenameTmp);
	}
	m_nVideoFrmNum=0;

	if(m_pEncCtx){
		aacEnc_exit(m_pEncCtx);
		m_pEncCtx = NULL;
	}
	return nRet;
}
//----}}MP4----------------------------------------------------------------------------------------------







//----{{RTSP---------------------------------------------------------------------------------------------
#define RTSP_CONNECT_TIMEOUT_MSEC	5000
#define MAX_SIZE_PLAYBACK_BUF		384*1024
#define ADTS_HEADER_LEN				7
static char  NAL_START[]				={ 0,0,0,1 };
static UCHAR ADTSHeader[ADTS_HEADER_LEN]={ 0xFF, 0xF9, 0x60, 0x40, 0x00, 0x1F, 0xFC};

RTSP::RTSP(RTSP_POnStreamCallback pCallback, void *pUserData)
{
	m_bRunning=0;
	m_dwOpenTime=0;
	m_pInputFormatContext=NULL;
	m_pStreamCallback=NULL;

	m_threadIDRead=0L;
	m_handleReadThread=NULL;
	
	m_nVideoIndex=m_nAudioIndex=-1;
	m_nDataIndex=-1;
	m_pBuf=NULL;
	m_pchUrl=(CHAR *)malloc(256);

	m_pCallBack=pCallback;
	m_pUserData=pUserData;
	av_register_all();
	avformat_network_init();

	pub_mutex_init(m_mutexWrite);
}

RTSP::~RTSP()
{
	RTSP_CloseUrl();
	if(m_pchUrl){
		free(m_pchUrl);
		m_pchUrl=NULL;
	}
	avformat_network_deinit();
	pub_mutex_destroy(m_mutexWrite);
}

//static 
UINT32 RTSP::RTSP_GetSdkVer(char *chDesc, INT32 nDescMaxSize)
{
	return SEMP4_GetSdkVer(chDesc, nDescMaxSize);
}

handle_thread WINAPI RTSP::ThreadReadStrem(void* parameter)
{
	RTSP *pTHIS=(RTSP *)parameter;
	pTHIS->DoReadStream();
	return 0;
}

int RTSP::GetIndexBy(int nSampleRate)
{
	int nIndex=11;
	switch(nSampleRate){
		case 96000:
			nIndex=0;
			break;

		case 88200:
			nIndex=1;
			break;

		case 64000:
			nIndex=2;
			break;

		case 48000:
			nIndex=3;
			break;

		case 44100:
			nIndex=4;
			break;

		case 32000:
			nIndex=5;
			break;

		case 24000:
			nIndex=6;
			break;

		case 22050:
			nIndex=7;
			break;

		case 16000:
			nIndex=8;
			break;

		case 12000:
			nIndex=9;
			break;

		case 11025:
			nIndex=10;
			break;

		case 8000:
			nIndex=11;
			break;

		case 7350:
			nIndex=12;
			break;
		default:;
	}
	return nIndex;
}

//UINT32 g_nPTS=0;
//int64_t g_nPTS64=0;
void RTSP::DoReadStream()
{
	INT32  nRet=0, nReadRet=0;
	INT32  nAudioFrameSize=0, nSampleIndex=11;
	UINT32 nPTS=0;
	UCHAR *pRawData=NULL;
	AVRational stTimeBaseUs = {1, 1000};
	AVCodecContext  *pAVCodecC=NULL;
	AVPacket* pPacket = new AVPacket;
	av_init_packet(pPacket);

	if(m_pBuf==NULL) m_pBuf=(UCHAR *)malloc(MAX_SIZE_PLAYBACK_BUF);
	RTSP_STREAM_HEAD *pHead=(RTSP_STREAM_HEAD *)m_pBuf;
	pRawData=m_pBuf+sizeof(RTSP_STREAM_HEAD);

	TRACE("CTestRecMp4Dlg::DoReadStream() is going...\n");
	AVDictionary *pAVOptions =NULL;

	//CHAR chTmp[256]={0};
	while(m_bRunning){
		nReadRet = av_read_frame(m_pInputFormatContext, pPacket);
		if(nReadRet==AVERROR_EOF){ //AVERROR_EOF = -541478725
			mp4pub_Sleep(500);

			if(m_pInputFormatContext->duration == AV_NOPTS_VALUE){
		AGAIN_CONNECT:
				if(!m_bRunning) break;
				else{
					//reconnect
					av_dict_set(&pAVOptions, "stimeout", "5000000", 0); //RTSP_CONNECT_TIMEOUT_MSEC
					av_dict_set(&pAVOptions, "rtsp_transport","tcp", 0);
					av_dict_set(&pAVOptions, "user-agent", "HuiYanCompany", 0);
					//av_dict_set(&pAVOptions, "initial_pause", "1", 0); //invalid for mp4 file
					if(m_pInputFormatContext){
						avformat_close_input(&m_pInputFormatContext);
						m_pInputFormatContext = NULL;
					}
					if(!m_pInputFormatContext){
						m_pInputFormatContext = avformat_alloc_context();
						m_pInputFormatContext->interrupt_callback.callback = AvFormatOpenCallback;
						m_pInputFormatContext->interrupt_callback.opaque = this;
					}
					m_dwOpenTime = mp4pub_GetTickCount();
					nRet = avformat_open_input(&m_pInputFormatContext, m_pchUrl, NULL, &pAVOptions);
					if(nRet != 0){
						TRACE("rtsp:Failed to connect\n");
						goto AGAIN_CONNECT;
					}
				}
			}
			continue;

		}else if(nReadRet<0){
			mp4pub_Sleep(500);
			continue;
		}
		if(nReadRet>=0){
			if(pPacket->stream_index == m_nVideoIndex){
				nPTS = (UINT32)av_rescale_q(pPacket->pts, m_pInputFormatContext->streams[m_nVideoIndex]->time_base, stTimeBaseUs);
				/*
				if(nPTS<g_nPTS)
				{
					sprintf(chTmp, "----------nPTS=%d g_nPTS=%d pPacket->size=%d pPacket->pts=%I64d g_nPTS64=%I64d\n", nPTS, g_nPTS, pPacket->size, pPacket->pts, g_nPTS64);
					OutputDebugStringA(chTmp);
				}
				g_nPTS=nPTS;
				g_nPTS64=pPacket->pts;
				*/

				if((pPacket->flags & AV_PKT_FLAG_KEY)== AV_PKT_FLAG_KEY)
				{
					//I frame
					pAVCodecC=m_pInputFormatContext->streams[m_nVideoIndex]->codec;
					memcpy(pRawData, pAVCodecC->extradata, pAVCodecC->extradata_size);
					memcpy(pRawData+pAVCodecC->extradata_size, pPacket->data, pPacket->size);

					//TRACE("doPlayback] I, sendPlaybackData(.)=%d size=%d nPTS=%d 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X \n", 
					//	  nReadRet, pPacket->size, nPTS,
					//	  pPacket->data[0], pPacket->data[1], pPacket->data[2], pPacket->data[3],pPacket->data[4], pPacket->data[5]);
					if(m_pCallBack){
						if(m_pInputFormatContext->streams[m_nVideoIndex]->codec->codec_id==AV_CODEC_ID_H264) pHead->nAVCodecID=RTSP_AV_CODECID_VIDEO_H264;
						else if(m_pInputFormatContext->streams[m_nVideoIndex]->codec->codec_id==AV_CODEC_ID_MJPEG) pHead->nAVCodecID=RTSP_AV_CODECID_VIDEO_MJPEG;
						else pHead->nAVCodecID=RTSP_AV_CODECID_UNKNOWN;
						pHead->nParameter=RTSP_VIDEO_FRAME_FLAG_I;
						pHead->nTimestamp=nPTS;
						pHead->nStreamDataLen=pAVCodecC->extradata_size+pPacket->size;

						m_pCallBack(m_pBuf, sizeof(RTSP_STREAM_HEAD)+pHead->nStreamDataLen, m_pUserData);
					}
					if(nReadRet==-24){
						//exception
						break;
					}
				}else{
					//P frame
					memcpy(pRawData, pPacket->data, pPacket->size);
					//nReadRet=sendPlaybackData(AV_V_FRAME_P, pTmpBuf, pPacket->size, nPTS, nPlaybackID);
					//TRACE("doPlayback] %d P, sendPlaybackData(.)=%d size=%d nPTS=%d 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X \n", 
					//		i++, nReadRet,pPacket->size, nPTS,
					//		pPacket->data[0], pPacket->data[1], pPacket->data[2], pPacket->data[3],pPacket->data[4], pPacket->data[5]);
					if(m_pCallBack){
						if(m_pInputFormatContext->streams[m_nVideoIndex]->codec->codec_id==AV_CODEC_ID_H264) pHead->nAVCodecID=RTSP_AV_CODECID_VIDEO_H264;
						else if(m_pInputFormatContext->streams[m_nVideoIndex]->codec->codec_id==AV_CODEC_ID_MJPEG) pHead->nAVCodecID=RTSP_AV_CODECID_VIDEO_MJPEG;
						else pHead->nAVCodecID=RTSP_AV_CODECID_UNKNOWN;
						pHead->nParameter=RTSP_VIDEO_FRAME_FLAG_P;
						pHead->nTimestamp=nPTS;
						pHead->nStreamDataLen=pPacket->size;

// 						if(gFP==NULL) gFP=fopen("d:\\qvga_mp4.h264", "wb");
// 						if(gFP && gnCount<200){
// 							fwrite(pRawData, 1, pHead->nStreamDataLen, gFP);
// 							fflush(gFP);
// 							gnCount++;
// 						}
// 						if(gnCount==200){
// 							fclose(gFP);
// 						}
						m_pCallBack(m_pBuf, sizeof(RTSP_STREAM_HEAD)+pHead->nStreamDataLen, m_pUserData);
					}
					if(nReadRet==-24){
						//exception
						break;
					}
				}
			}else if(pPacket->stream_index == m_nAudioIndex){
				nPTS = (INT32)av_rescale_q(pPacket->pts, m_pInputFormatContext->streams[m_nAudioIndex]->time_base, stTimeBaseUs);
				pAVCodecC=m_pInputFormatContext->streams[m_nAudioIndex]->codec;
				if(pAVCodecC->codec_id==AV_CODEC_ID_AAC){ //AAC
					//pAACAdts->sampling_frequency_index=((pTmp[2]&0x3C)>>2)&0x0F; //4
					//
					nSampleIndex=GetIndexBy(pAVCodecC->sample_rate);
					nSampleIndex=(nSampleIndex<<2)&0x2C;
					ADTSHeader[2]=ADTSHeader[2] | nSampleIndex;

					nAudioFrameSize= ADTS_HEADER_LEN+pPacket->size;
					ADTSHeader[3] = 0x40 | ((nAudioFrameSize & 0xFFFF) >> 11);
					ADTSHeader[4] = (nAudioFrameSize & 0x7FF) >> 3;
					ADTSHeader[5] = 0x1F | ((nAudioFrameSize & 0x0007) << 5);

					memcpy(pRawData, ADTSHeader, ADTS_HEADER_LEN);
					memcpy(pRawData+ADTS_HEADER_LEN, pPacket->data, pPacket->size);

					pHead->nAVCodecID=RTSP_AV_CODECID_AUDIO_AAC;
					pHead->nParameter=0x0E;
					if(m_pCallBack){
						pHead->nTimestamp=nPTS;
						pHead->nStreamDataLen=ADTS_HEADER_LEN+pPacket->size;
						m_pCallBack(m_pBuf, sizeof(RTSP_STREAM_HEAD)+ADTS_HEADER_LEN+pPacket->size, m_pUserData);
					}

				}else if(pAVCodecC->codec_id==AV_CODEC_ID_PCM_ALAW){ //G711A
					memcpy(pRawData, pPacket->data, pPacket->size);
					pHead->nAVCodecID=RTSP_AV_CODECID_AUDIO_G711A;
					pHead->nParameter=0x2;
					if(m_pCallBack){
						pHead->nTimestamp=nPTS;
						pHead->nStreamDataLen=pPacket->size;
						m_pCallBack(m_pBuf, sizeof(RTSP_STREAM_HEAD)+pPacket->size, m_pUserData);
					}
				}else pHead->nAVCodecID=RTSP_AV_CODECID_UNKNOWN;
				
				//TRACE("\tdoPlayback] audio, sendPlaybackData(.)=%d size=%d nPTS=%d\n", nReadRet, pPacket->size, nPTS);
				if(nReadRet==-24){
					//exception
					break;
				}
			}else if(pPacket->stream_index == m_nDataIndex){
				//如果同时有多种报警内容，则报警数据示例内容如下：
				//MB1-10-100-20-200-1920-1080|MB2-10-100-20-200-1920-1080|alarmin:1|audioin:1
				//心跳数据内容（注意：心跳包间隔时间是1秒）: Keepalive

				//RTSP_AV_CODECID_DATA_ALARM
				if(pPacket->size>0){
					uint8_t* pAlarmData=pPacket->data;
					if(pAlarmData[0]=='k' && pAlarmData[1]=='e' && pAlarmData[2]=='e');
					else if(m_pCallBack){
						memcpy(pRawData, pPacket->data, pPacket->size);
						pHead->nAVCodecID=RTSP_AV_CODECID_DATA_ALARM;
						pHead->nTimestamp=nPTS;
						pHead->nStreamDataLen=pPacket->size;
						m_pCallBack(m_pBuf, sizeof(RTSP_STREAM_HEAD)+pPacket->size, m_pUserData);
					}
				}
			}
			if(pPacket) av_free_packet(pPacket);

		}else {

		}
	}

FUN_END:
	if(pAVOptions!=NULL){
		av_dict_free(&pAVOptions);
		pAVOptions=NULL;
	}
	if(pPacket){
		av_free_packet(pPacket);
		delete pPacket;
	}
	TRACE("CTestRecMp4Dlg::DoReadStream() is over\n");	
}

int RTSP::IsReconnect()
{
	int nRet = 0;
	int nSpanMs = mp4pub_GetTickCount() - m_dwOpenTime;
	if(GetStreamNum() <= 0 && nSpanMs > RTSP_CONNECT_TIMEOUT_MSEC)
	{
		nRet = 1; //abort reconnecting
		TRACE("----RTSP::IsReconnect(), abort reconnecting\n");
		//m_bRunning=0;
	}
	return nRet;
}

int RTSP::AvFormatOpenCallback(void* param)
{
	RTSP* pPlayer = (RTSP*)param;
	//If the callback returns 1, the blocking operation will be aborted
	return pPlayer->IsReconnect();
}

INT32 RTSP::RTSP_ResetUrl(char *pchUrl)
{
	if(pchUrl==NULL) return -1;
	strcpy(m_pchUrl, pchUrl);
	return 0;
}

INT32 RTSP::RTSP_OpenUrl(char *pchUrl, char *pchKeyValue)
{
	INT32 i=0;
	AVDictionary *pAVOptions =NULL;

	if(pchUrl==NULL) return -1;
	strcpy(m_pchUrl, pchUrl);

pub_mutex_lock(m_mutexWrite);
	av_dict_set(&pAVOptions, "stimeout", "5000000", 0); //RTSP_CONNECT_TIMEOUT_MSEC
	av_dict_set(&pAVOptions, "rtsp_transport","tcp", 0);
	av_dict_set(&pAVOptions, "user-agent", "HuiYanCompany", 0);
	//av_dict_set(&pAVOptions, "initial_pause", "1", 0); //invalid for mp4 file
	if(!m_pInputFormatContext) {
		m_pInputFormatContext = avformat_alloc_context();
		m_pInputFormatContext->interrupt_callback.callback = AvFormatOpenCallback;
		m_pInputFormatContext->interrupt_callback.opaque = this;
	}
	m_dwOpenTime = mp4pub_GetTickCount();
	i = avformat_open_input(&m_pInputFormatContext, pchUrl, NULL, &pAVOptions);
	if(pAVOptions!=NULL){
		av_dict_free(&pAVOptions);
		pAVOptions=NULL;
	}
	TRACE("avformat_open_input()=%d\n", i);
	if(i != 0){
		if(i==AVERROR_INVALIDDATA){
			TRACE("rtsp:Wrong user or password.\n");
		}else{
			TRACE("rtsp:Failed to connect\n");
		}
		pub_mutex_unlock(m_mutexWrite);
		return -2;
	}
	for(i=0; i<m_pInputFormatContext->nb_streams; i++){
		if(m_pInputFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) m_nVideoIndex = i;
		else if(m_pInputFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)m_nAudioIndex = i;
		else if(m_pInputFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_DATA) m_nDataIndex = i;
	}
	m_bRunning=1;
	mp4pub_CreateThread(ThreadReadStrem, (void*)this, 0,0, &m_handleReadThread, &m_threadIDRead);
	//av_read_play(m_pInputFormatContext); //会卡住
pub_mutex_unlock(m_mutexWrite);

	return 0;
}

INT32 RTSP::RTSP_CloseUrl()
{
pub_mutex_lock(m_mutexWrite);
	m_bRunning=0;
	if(m_handleReadThread){
		pub_thread_join(m_handleReadThread, m_threadIDRead);
		m_handleReadThread=NULL;
		m_threadIDRead = NULL;
	}

	if(m_pInputFormatContext){
		avformat_close_input(&m_pInputFormatContext);
		m_pInputFormatContext = NULL;
	}
	if(m_pBuf){
		free(m_pBuf);
		m_pBuf=NULL;
	}
pub_mutex_unlock(m_mutexWrite);
	return 0;	
}

//----}}RTSP---------------------------------------------------------------------------------------------




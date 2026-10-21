#include "hyBaseType.h"
#include "audio_aac.h"

	#define MY_PRINT printf

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
	#pragma comment(lib,"User32.lib")
#endif


#define	AUDIO_HZ			8000 //16000
#define	CAP_AUDIO_SAMPLE_FMT	AV_SAMPLE_FMT_S16
#define	ENC_AUDIO_SAMPLE_FMT	AV_SAMPLE_FMT_FLTP
#define AUDIO_INBUF_SIZE	1024 //for AAC
#define MAX_SIZE_IN_BUF		2048
#define	AUDIO_BUFFER_SAMPLE	4096 //for AAC

AAC_ENC_CONTEXT *aacEnc_init(int nSampleRate,int nChn,int nSampleBits,int nBitRate){
		int ret=0;
		AAC_ENC_CONTEXT *pAACEncContext=(AAC_ENC_CONTEXT *)malloc(sizeof(AAC_ENC_CONTEXT));
		memset(pAACEncContext, 0, sizeof(AAC_ENC_CONTEXT));
		pAACEncContext->m_nSampleRate=8000;
		
		//av_register_all();
		//avcodec_register_all();
		pAACEncContext->m_pEncodeCodec=avcodec_find_encoder(AV_CODEC_ID_AAC);
		if(pAACEncContext->m_pEncodeCodec==NULL) MY_PRINT("Codec encoder not found AV_CODEC_ID_AAC\n");
		pAACEncContext->m_pEncodeCodecCtx=avcodec_alloc_context3(pAACEncContext->m_pEncodeCodec);
		if(pAACEncContext->m_pEncodeCodecCtx==NULL) MY_PRINT("Could not allocate audio AAC encoder context\n");

		pAACEncContext->m_pEncodeCodecCtx->codec = pAACEncContext->m_pEncodeCodec;
		if(pAACEncContext->m_pEncodeCodecCtx->codec->capabilities & AV_CODEC_CAP_TRUNCATED){
			pAACEncContext->m_pEncodeCodecCtx->flags |= AV_CODEC_FLAG_TRUNCATED;
			MY_PRINT("\tAudioManagerTool] CODEC_FLAG_TRUNCATED\n");
		}

		if(nSampleRate<AUDIO_HZ) nSampleRate=AUDIO_HZ;
		int nChanLayout=AV_CH_LAYOUT_MONO;
		if(nChn>=2) nChanLayout=AV_CH_LAYOUT_STEREO;
		pAACEncContext->m_nSampleRate=nSampleRate;
		
		pAACEncContext->m_pEncodeCodecCtx->bit_rate		= nSampleRate<<1;
		pAACEncContext->m_pEncodeCodecCtx->sample_rate	= nSampleRate;
		pAACEncContext->m_pEncodeCodecCtx->channels		= nChn;
		pAACEncContext->m_pEncodeCodecCtx->channel_layout = nChanLayout;
		pAACEncContext->m_pEncodeCodecCtx->sample_fmt	= ENC_AUDIO_SAMPLE_FMT;
		pAACEncContext->m_pEncodeCodecCtx->profile		= FF_PROFILE_AAC_LOW;
		pAACEncContext->m_pEncodeCodecCtx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
		if(avcodec_open2(pAACEncContext->m_pEncodeCodecCtx, pAACEncContext->m_pEncodeCodec, NULL) < 0)  MY_PRINT( "Could not open encoder codec\n");
		int cap = (pAACEncContext->m_pEncodeCodec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE);

		pAACEncContext->m_pEncodeSwt=swr_alloc();
		// set options
		av_opt_set_int(pAACEncContext->m_pEncodeSwt, "in_channel_layout",		nChanLayout, 0);
		av_opt_set_int(pAACEncContext->m_pEncodeSwt, "in_sample_rate",			nSampleRate, 0);
		av_opt_set_sample_fmt(pAACEncContext->m_pEncodeSwt, "in_sample_fmt",	CAP_AUDIO_SAMPLE_FMT, 0);
		av_opt_set_int(pAACEncContext->m_pEncodeSwt, "out_channel_layout",		nChanLayout, 0);
		av_opt_set_int(pAACEncContext->m_pEncodeSwt, "out_sample_rate",			nSampleRate, 0);
		av_opt_set_sample_fmt(pAACEncContext->m_pEncodeSwt, "out_sample_fmt",	ENC_AUDIO_SAMPLE_FMT, 0);
		swr_init(pAACEncContext->m_pEncodeSwt);

		pAACEncContext->m_pEncodedFrame=av_frame_alloc();

		int nCapLineSize = 0, nEncLineSize = 0;
		int nAudio_cap_size = av_samples_get_buffer_size(&nCapLineSize, nChn, pAACEncContext->m_pEncodeCodecCtx->frame_size, CAP_AUDIO_SAMPLE_FMT, 1); //2048
		int nAudio_enc_size = av_samples_get_buffer_size(&nEncLineSize, nChn, pAACEncContext->m_pEncodeCodecCtx->frame_size, ENC_AUDIO_SAMPLE_FMT, 1); //4096
		pAACEncContext->m_nEncodeBufSizeSwr=nAudio_enc_size;
		pAACEncContext->m_pEncodeBufSwr=(HUCHAR *)av_malloc(pAACEncContext->m_nEncodeBufSizeSwr);
		pAACEncContext->m_pEncodeBufResult  =(HUCHAR *)av_malloc(AUDIO_INBUF_SIZE);
		return pAACEncContext;
	}

	int aacEnc_exit(AAC_ENC_CONTEXT *pAACEncContext){
		int nRet = 0;
		if(pAACEncContext->m_pEncodeSwt){
			swr_free(&pAACEncContext->m_pEncodeSwt);
			pAACEncContext->m_pEncodeSwt=NULL;
		}
		if(pAACEncContext->m_pEncodedFrame) {
			av_frame_free(&pAACEncContext->m_pEncodedFrame);
			pAACEncContext->m_pEncodedFrame=NULL;
		}

		if(pAACEncContext->m_pEncodeBufResult){
			av_free(pAACEncContext->m_pEncodeBufResult);
			pAACEncContext->m_pEncodeBufResult=NULL;
		}
		if(pAACEncContext->m_pEncodeBufSwr){
			av_free(pAACEncContext->m_pEncodeBufSwr);
			pAACEncContext->m_pEncodeBufSwr=NULL;
		}

		return nRet;
	}
	
	#define AAC_ADTS_HEAD_LEN	7
	static HINT32 g_nSampleRate[] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350};
	void  aacEnc_makeHead(HCHAR *pBufOut, HINT32 nSampleRate, HINT32 nSizeOfES){
		int nProfile = 2; // AAC LC
		int nChanCfg = 1;
		static int nFreqIdx=0;
		if(nFreqIdx==0){
			int i=0;
			for(i=0; i<(int)(sizeof(g_nSampleRate)/sizeof(HINT32)); i++){
				if(nSampleRate==g_nSampleRate[i]){
					nFreqIdx=i;
					break;
				}
			}
		}
		// fill in ADTS data
		pBufOut[0] = (HUCHAR)0xFF;
		pBufOut[1] = (HUCHAR)0xF9;
		pBufOut[2] = (HUCHAR)(((nProfile - 1) << 6) + (nFreqIdx << 2) + (nChanCfg >> 2));
		pBufOut[3] = (HUCHAR)(((nChanCfg & 3) << 6) + (nSizeOfES >> 11));
		pBufOut[4] = (HUCHAR)((nSizeOfES & 0x7FF) >> 3);
		pBufOut[5] = (HUCHAR)(((nSizeOfES & 7) << 5) + 0x1F);
		pBufOut[6] = (HUCHAR)0xFC;
	}
	void  aacEnc_updateHead(HCHAR *pBufOut, HINT32 nSizeOfES){
		pBufOut[3] = 0x40 | ((nSizeOfES & 0xFFFF) >> 11);
		pBufOut[4] = (nSizeOfES & 0x7FF) >> 3;
		pBufOut[5] = 0x1F | ((nSizeOfES & 0x0007) << 5);
	}
	
	//return:
	//	>0: ok
	// <=0: fail
	int	aacEnc_frame_with_adts_head(AAC_ENC_CONTEXT *pAACEncContext, unsigned char *pSrc, unsigned int srclen, unsigned char *pDest, int *pDstlen){
		#define ADTS_HEADER_LEN 7
		static HUCHAR ADTSHeader[ADTS_HEADER_LEN] = { 0xFF, 0xF9, 0x6C, 0x40, 0x00, 0x1F, 0xFC };
		//														 0x60 16K samplerate
		//														 0x6C  8K samplerate
		int nRetTmp=0, got_packet=0, nFrameSize=0;
		int ret = 0;

		if(pAACEncContext->m_nSampleRate==8000) ADTSHeader[2]=0x6c;
		else if(pAACEncContext->m_nSampleRate==16000) ADTSHeader[2]=0x60;

		av_init_packet(&pAACEncContext->m_stEncodeAVpkt);
		pAACEncContext->m_stEncodeAVpkt.data = pAACEncContext->m_pEncodeBufResult;
		pAACEncContext->m_stEncodeAVpkt.size = AUDIO_INBUF_SIZE;

		pAACEncContext->m_pEncodedFrame->nb_samples = pAACEncContext->m_pEncodeCodecCtx->frame_size;
		pAACEncContext->m_pEncodedFrame->format		= pAACEncContext->m_pEncodeCodecCtx->sample_fmt;
		pAACEncContext->m_pEncodedFrame->channel_layout = pAACEncContext->m_pEncodeCodecCtx->channel_layout;
		pAACEncContext->m_pEncodedFrame->pts = AV_NOPTS_VALUE;
		
		HUCHAR *pAdtsHeader=pDest;
		HUCHAR *pAACData=pDest+ADTS_HEADER_LEN;
		memcpy(pAdtsHeader, ADTSHeader, ADTS_HEADER_LEN);

		got_packet=0;
		nRetTmp = swr_convert(pAACEncContext->m_pEncodeSwt, (uint8_t**)&pAACEncContext->m_pEncodeBufSwr, pAACEncContext->m_pEncodedFrame->nb_samples, (const uint8_t**)&pSrc, pAACEncContext->m_pEncodedFrame->nb_samples);
		//AVERROR
		nRetTmp = avcodec_fill_audio_frame(pAACEncContext->m_pEncodedFrame, pAACEncContext->m_pEncodeCodecCtx->channels, pAACEncContext->m_pEncodeCodecCtx->sample_fmt, (const uint8_t*)pAACEncContext->m_pEncodeBufSwr, pAACEncContext->m_nEncodeBufSizeSwr, 0);
		nRetTmp = avcodec_encode_audio2(pAACEncContext->m_pEncodeCodecCtx, &pAACEncContext->m_stEncodeAVpkt, pAACEncContext->m_pEncodedFrame, &got_packet);
		
		#ifdef LINUX_ANDROID
			//LOGI("SEAudio 0x%02X %02X %02X %02X \n", pSrc[0]&0xFF,pSrc[1]&0xFF,pSrc[2]&0xFF,pSrc[3]&0xFF);
			//LOGI("SEAudio m_nEncAssem=%d inLen=%d *outLen=%d nRetTmp=%d got_packet=%d pAACEncContext->m_stEncodeAVpkt.size=%d\n", m_nEncAssem, inLen, *p_outLen, nRetTmp,got_packet, pAACEncContext->m_stEncodeAVpkt.size);
		#endif

		if(got_packet > 0){
			memcpy(pAACData+nFrameSize, pAACEncContext->m_stEncodeAVpkt.data, pAACEncContext->m_stEncodeAVpkt.size);
			nFrameSize += pAACEncContext->m_stEncodeAVpkt.size;

			pAACEncContext->m_stEncodeAVpkt.data = pAACEncContext->m_pEncodeBufResult;
			pAACEncContext->m_stEncodeAVpkt.size = AUDIO_INBUF_SIZE;
			ret=1;
		}

		if(ret>0){
			nFrameSize = nFrameSize+ ADTS_HEADER_LEN;
			pAdtsHeader[3] = 0x40 | ((nFrameSize & 0xFFFF) >> 11);
			pAdtsHeader[4] = (nFrameSize & 0x7FF) >> 3;
			pAdtsHeader[5] = 0x1F | ((nFrameSize & 0x0007) << 5);
			if(pDstlen) *pDstlen=nFrameSize;
		}
	FUN_END:
		return ret;
	}

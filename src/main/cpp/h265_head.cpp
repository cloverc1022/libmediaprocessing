
#include "SEP2P_Type.h"
#include "h265_head.h"
#include "assert.h"

typedef enum e_hevc_nalu_type{
	HEVC_NAL_TRAIL_N    = 0,	//B SLICE
	HEVC_NAL_TRAIL_R    = 1,	//P SLICE
	HEVC_NAL_TSA_N      = 2,	//
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

	HEVC_NAL_BLA_W_LP   = 16,	//
	HEVC_NAL_BLA_W_RADL = 17,
	HEVC_NAL_BLA_N_LP   = 18,
	HEVC_NAL_IDR_W_RADL = 19,	//IDR
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
	HEVC_NAL_VPS        = 32,	//VPS
	HEVC_NAL_SPS        = 33,	//SPS
	HEVC_NAL_PPS        = 34,	//PPS
	HEVC_NAL_AUD        = 35,
	HEVC_NAL_EOS_NUT    = 36,
	HEVC_NAL_EOB_NUT    = 37,
	HEVC_NAL_FD_NUT     = 38,
	HEVC_NAL_SEI_PREFIX = 39,	//SEI
	HEVC_NAL_SEI_SUFFIX = 40	//SEI
}E_HEVC_NALU_TYPE;



typedef struct t_h265_nalu_header{
	UINT16 forbidden_zero_bit:1, nal_unit_type:6, nuh_layer_id:6, nuh_temporal_id_plus1:3;
}T_H265_NALU_HEADER;

#define H265_NAL_TYPE(v)	((v>>1) & 0x3f)
static inline int h265NALType(const unsigned char* ptr)
{
	int i=2;
	assert(0x00 == ptr[0] && 0x00 == ptr[1]);
	if(0x00 == ptr[2])	++i;
	assert(0x01 == ptr[i]);
	return H265_NAL_TYPE(ptr[i + 1]);
}

UINT32 h265FindStartCode(UCHAR *pBuf, UINT32 zeros_in_startcode)   
{
	UINT32 info=0;
	UINT32 i=0;

	info = 1;
	if((info = (pBuf[zeros_in_startcode]!=1) ? 0 : 1)== 0) return 0;

	for(i=0; i<zeros_in_startcode; i++)
		if(pBuf[i] != 0){
			info = 0;
			break;
		};
	return info;
}

//parameter:
//		ppOffset: 当函数返回NULL时，指向最后1个不完整的NAL
//return:
//		
UCHAR *h265GetNAL(UINT32 *pnLen, UCHAR **ppOffset, UCHAR *pStart, UINT32 nTotal)
{
	UINT32 info=0;
	UCHAR *q=NULL;
	UCHAR *p=*ppOffset;
	*pnLen = 0;

	while(1){ //search 1st startcode
		info =  h265FindStartCode(p, 3);
		if(info == 1) break;
		p++;
		if((p - pStart) >= nTotal) return NULL;
	}

	q = p+4;
	//if((*q&0x1F)==E_NAL_IDR) return NULL;

	p = q;
	while(1){ //search 2nd startcode
		info =  h265FindStartCode(p, 3);
		if(info == 1) break;

		p++;
		if((p - pStart) >= nTotal) return NULL;
	}

	*pnLen = (p - q);
	*ppOffset = p;
	return q;
}

UINT32 h265GetSps(UCHAR *pRaw, UINT32 nDataLen, INT32 bIFrame, UCHAR *pSps)
{
	INT32  nSpsLen=0, nOk=0, nNALType=0;
	UCHAR  *pNAL1=NULL; //NAL of removing start code
	UCHAR  *pNALfirst=NULL; 
	UINT32 nNALTotal=0, nNalLen=0;
	UCHAR  *pOffsetVideo = pRaw;
	UCHAR  bOver=0;
	if(pSps==NULL) return 0;

	while(!bOver && bIFrame){
		//get one NAL
		pNAL1=h265GetNAL(&nNalLen, &pOffsetVideo, pRaw, nDataLen);
		if(pNAL1==NULL){
			if(h265FindStartCode(pOffsetVideo,3)==1){
				//found one NAL
				pNAL1=pOffsetVideo;
				nNalLen=nDataLen - nNALTotal;
				bOver=1;
			}else break;
		}else{//found one NAL
			pNAL1-=4; //4 is start code
			nNalLen+=4;
			nNALTotal+=nNalLen;
		}
		nNALType=h265NALType(pNAL1);
		switch(nNALType){
			case HEVC_NAL_SPS:{
				nOk++;
				memcpy((char *)(pSps+nSpsLen), pNAL1, nNalLen);
				nSpsLen+=nNalLen;
				}break;
			default:;
		}
		if(nOk>=1) break;
	}
	return nSpsLen;
}

UINT32 h265GetVpsSpsPps(UCHAR *pRaw, UINT32 nDataLen, INT32 bIFrame, UCHAR *pSpsPps)
{
	INT32  nSpsPpsLen=0, nOk=0, nNALType=0;
	UCHAR  *pNAL1=NULL; //NAL of removing start code
	UCHAR  *pNALfirst=NULL; 
	UINT32 nNALTotal=0, nNalLen=0;
	UCHAR  *pOffsetVideo = pRaw;
	UCHAR  bOver=0;
	if(pSpsPps==NULL) return 0;

	while(!bOver && bIFrame){
		//get one NAL
		pNAL1=h265GetNAL(&nNalLen, &pOffsetVideo, pRaw, nDataLen);
		if(pNAL1==NULL){
			if(h265FindStartCode(pOffsetVideo,3)==1){
				//found one NAL
				pNAL1=pOffsetVideo;
				nNalLen=nDataLen - nNALTotal;
				bOver=1;
			}else break;

		}else{//found one NAL
			pNAL1-=4; //4 is start code
			nNalLen+=4;
			nNALTotal+=nNalLen;
			if(pNALfirst==NULL) pNALfirst=pNAL1;
		}

		nNALType=h265NALType(pNAL1);
		switch(nNALType){
			case HEVC_NAL_IDR_W_RADL:{ //stop at next nal type
				nOk++;
				memcpy((char *)(pSpsPps+nSpsPpsLen), pNALfirst, nNALTotal);
				nSpsPpsLen=nNALTotal;
				}break;
			default:;
		}
		if(nOk>=1) break;
	}
	return nSpsPpsLen;
}

class NALBitstream
{
public:
	NALBitstream() : m_data(NULL), m_len(0), m_idx(0), m_bits(0), m_byte(0), m_zeros(0) { };
	NALBitstream(void * data, INT32 len){	Init(data, len); };
	void Init(void * data, INT32 len){
		m_data = (UCHAR *)data;
		m_len  = len;
		m_idx  = 0; 
		m_bits = 0;
		m_byte = 0; 
		m_zeros= 0; 
	};
	UCHAR GetBYTE(){
		if(m_idx >= m_len) return 0;
		UCHAR b = m_data[m_idx++];
		if ( b == 0 ){
			m_zeros++;
			if((m_idx < m_len) && (m_zeros == 2) && (m_data[m_idx] == 0x03)){
				m_idx++;
				m_zeros=0;
			}
		}else  m_zeros = 0;

		return b;
	};

	UINT32 GetBit() {
		if (m_bits == 0){
			m_byte = GetBYTE();
			m_bits = 8;
		}
		m_bits--;
		return (m_byte >> m_bits) & 0x1;
	};

	UINT32 GetWord(INT32 bits) 
	{
		UINT32 u = 0;
		while ( bits > 0){
			u <<= 1;
			u |= GetBit();
			bits--;
		}
		return u;
	};

	UINT32 GetUE() 
	{
		INT32 zeros = 0;
		while (m_idx < m_len && GetBit() == 0 ) zeros++;
		return GetWord(zeros) + ((1 << zeros) - 1);
	};

	INT32 GetSE()
	{
		UINT32 UE = GetUE();
		bool positive = UE & 1;
		INT32 SE = (UE + 1) >> 1;
		if ( !positive ){
			SE = -SE;
		}
		return SE;
	};
private:
	UCHAR *m_data;
	INT32 m_len;
	INT32 m_idx;
	INT32 m_bits;
	UCHAR m_byte;
	INT32 m_zeros;
};

INT32  h265ParseSeqParaSet(UCHAR* data,INT32 size, vc_params_t *pParams)
{
	if (size < 20)	return 0;
	
	NALBitstream bs(data, size);
	// seq_parameter_set_rbsp()
	bs.GetWord(4);// sps_video_parameter_set_id
	INT32 sps_max_sub_layers_minus1 = bs.GetWord(3); 
	if (sps_max_sub_layers_minus1 > 6) return 0;
	
	bs.GetWord(1); 
	{
		bs.GetWord(2); 
		bs.GetWord(1); 
		pParams->profile = bs.GetWord(5); 
		bs.GetWord(32);//
		bs.GetWord(1);// 
		bs.GetWord(1);// 
		bs.GetWord(1);// 
		bs.GetWord(1);//  
		bs.GetWord(44);// 
		pParams->level   = bs.GetWord(8);// general_level_idc
		UCHAR sub_layer_profile_present_flag[6] = {0};
		UCHAR sub_layer_level_present_flag[6]   = {0};
		for (INT32 i = 0; i < sps_max_sub_layers_minus1; i++) {
			sub_layer_profile_present_flag[i]= bs.GetWord(1);
			sub_layer_level_present_flag[i]= bs.GetWord(1);
		}
		if (sps_max_sub_layers_minus1 > 0){
			for (INT32 i = sps_max_sub_layers_minus1; i < 8; i++) {
				UCHAR reserved_zero_2bits = bs.GetWord(2);
			}
		}
		for (INT32 i = 0; i < sps_max_sub_layers_minus1; i++) {
			if (sub_layer_profile_present_flag[i]) {
				bs.GetWord(2); 
				bs.GetWord(1); 
				bs.GetWord(5);
				bs.GetWord(32); 
				bs.GetWord(1); 
				bs.GetWord(1); 
				bs.GetWord(1); 
				bs.GetWord(1); 
				bs.GetWord(44); 
			}
			if (sub_layer_level_present_flag[i]) {
				bs.GetWord(8);// sub_layer_level_idc[i]
			}
		}
	}
	UINT32 sps_seq_parameter_set_id= bs.GetUE();  
	if (sps_seq_parameter_set_id > 15)	return 0;
	
	UINT32 chroma_format_idc = bs.GetUE();  
	if (sps_seq_parameter_set_id > 3)  return 0;
	
	if (chroma_format_idc == 3) {
		bs.GetWord(1);//  
	}
	pParams->width  = bs.GetUE(); // pic_width_in_luma_samples
	pParams->height  = bs.GetUE(); // pic_height_in_luma_samples
	if (bs.GetWord(1)) { 
		bs.GetUE();  
		bs.GetUE();  
		bs.GetUE();  
		bs.GetUE();   
	}
	UINT32 bit_depth_luma_minus8= bs.GetUE();
	UINT32 bit_depth_chroma_minus8= bs.GetUE();
	if (bit_depth_luma_minus8 != bit_depth_chroma_minus8) return 0;
	
	return 1;
}

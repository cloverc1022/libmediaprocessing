
#include "mpeg4ip_bitstream_api.h"
#include "mpeg4ip_bitstream.h"

void* C_BitstreamCreate(const uint8_t *buffer, uint32_t bit_len)
{
	CBitstream *pBS=new CBitstream(buffer, bit_len);
	return pBS;
}

void  C_BitstreamDestroy(void* thisC)
{
	if(!thisC) return;
	CBitstream *pBS=static_cast<CBitstream*>(thisC);
	delete pBS;
}

uint32_t  C_Bitstream_GetBits(void* thisC, uint32_t numBits)
{
	if(!thisC) return 0;
	uint32_t nRet=0;
	try{
		CBitstream *pBS=static_cast<CBitstream*>(thisC);
		nRet=pBS->GetBits(numBits);
	}catch(...){
		nRet=0;
	}
	return nRet;
}

int  C_Bitstream_bits_remain(void* thisC)
{
	if(!thisC) return 0;
	CBitstream *pBS=static_cast<CBitstream*>(thisC);
	return pBS->bits_remain();
}

uint32_t  C_Bitstream_PeekBits(void* thisC, uint32_t bits)
{
	if(!thisC) return 0;
	CBitstream *pBS=static_cast<CBitstream*>(thisC);
	return pBS->PeekBits(bits);
}


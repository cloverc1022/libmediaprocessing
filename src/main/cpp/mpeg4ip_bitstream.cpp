
#include "mpeg4ip_bitstream.h"


CBitstream::CBitstream(void) { m_verbose = 0;};
CBitstream::CBitstream(const uint8_t *buffer, uint32_t bit_len) {
    m_verbose = 0;
    init(buffer, bit_len);
  }
CBitstream::~CBitstream (void){}

void CBitstream::init(const uint8_t *buffer, uint32_t bit_len) {
    m_chDecBuffer = buffer;
    m_chDecBufferSize = bit_len;
    m_bBookmarkOn = 0;
    m_uNumOfBitsInBuffer = 0;
  }

void CBitstream::init(const char *buffer, uint32_t bit_len) {
    init((const uint8_t *)buffer, (uint32_t)bit_len);
  }

void CBitstream::init(const char *buffer, int bit_len) {
    init((const uint8_t *)buffer, (uint32_t)bit_len);
  }

void CBitstream::init(const uint8_t *buffer, int bit_len) {
    init(buffer, (uint32_t)bit_len);
  }

void CBitstream::init(const char *buffer, unsigned short bit_len) {
    init((const uint8_t *)buffer, (uint32_t)bit_len);
  }

void CBitstream::init(const uint8_t *buffer, unsigned short bit_len) {
    init(buffer, (uint32_t)bit_len);
  }

uint32_t CBitstream::GetBits(uint32_t numBits) {
    uint32_t retData;
    static const uint32_t msk[33] = {
      0x00000000, 0x00000001, 0x00000003, 0x00000007,
      0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
      0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
      0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
      0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
      0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
      0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
      0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
      0xffffffff
    };

    if (numBits > 32) {
      throw BITSTREAM_TOO_MANY_BITS;
    }
  
    if (numBits == 0) {
      return 0;
    }
	
    if (m_uNumOfBitsInBuffer >= numBits) {  // don't need to read from FILE
      m_uNumOfBitsInBuffer -= numBits;
      retData = m_chDecData >> m_uNumOfBitsInBuffer;
      // wmay - this gets done below...retData &= msk[numBits];
    } else {
		  uint32_t nbits;
		  nbits = numBits - m_uNumOfBitsInBuffer;
		  if (nbits == 32) retData = 0;
		  else retData = m_chDecData << nbits;
		  switch ((nbits - 1) / 8) {
		  case 3:
			nbits -= 8;
			if (m_chDecBufferSize < 8) throw BITSTREAM_PAST_END;
			retData |= *m_chDecBuffer++ << nbits;
			m_chDecBufferSize -= 8;
		// fall through
		  case 2:
			nbits -= 8;
			if (m_chDecBufferSize < 8) throw BITSTREAM_PAST_END;
			retData |= *m_chDecBuffer++ << nbits;
			m_chDecBufferSize -= 8;
		  case 1:
			nbits -= 8;
			if (m_chDecBufferSize < 8) throw BITSTREAM_PAST_END;
			retData |= *m_chDecBuffer++ << nbits;
			m_chDecBufferSize -= 8;
		  case 0:
			break;
		  }
      if (m_chDecBufferSize < nbits) {
		throw BITSTREAM_PAST_END;
      }
      m_chDecData = *m_chDecBuffer++;
      m_uNumOfBitsInBuffer = MIN(8, m_chDecBufferSize) - nbits;
      m_chDecBufferSize -= MIN(8, m_chDecBufferSize);
      retData |= (m_chDecData >> m_uNumOfBitsInBuffer) & msk[nbits];
    }
    if(m_verbose) printf("bits %d value %x\n", numBits, retData&msk[numBits]);
    return (retData & msk[numBits]);
  }

int CBitstream::getbits(uint32_t bits, uint32_t *retvalue) {
    try {
      *retvalue = GetBits(bits);
    } catch (...) {
      return -1;
    }
    return 0;
  }

int CBitstream::peekbits(uint32_t bits, uint32_t *retvalue) {
    int ret;
    bookmark(1);
    ret = getbits(bits, retvalue);
    bookmark(0);
    return (ret);
  }

uint32_t CBitstream::PeekBits(uint32_t bits) {
    uint32_t ret;
    bookmark(1);
    ret = GetBits(bits);
    bookmark(0);
    return ret;
  }

void CBitstream::bookmark(int bSet) {
    if (m_verbose) {
      printf("bookmark\n");
    }
    if (bSet) {
      m_uNumOfBitsInBuffer_bookmark = m_uNumOfBitsInBuffer;
      m_chDecBuffer_bookmark = m_chDecBuffer;
      m_chDecBufferSize_bookmark = m_chDecBufferSize;
      m_bBookmarkOn = 1;
      m_chDecData_bookmark = m_chDecData;
    } else {
      m_uNumOfBitsInBuffer = m_uNumOfBitsInBuffer_bookmark;
      m_chDecBuffer = m_chDecBuffer_bookmark;
      m_chDecBufferSize = m_chDecBufferSize_bookmark;
      m_chDecData = m_chDecData_bookmark;
      m_bBookmarkOn = 0;
    }
  }

int CBitstream::bits_remain (void) {
    return m_chDecBufferSize + m_uNumOfBitsInBuffer;
  }

int CBitstream::byte_align(void) {
    int temp = 0;
    if (m_uNumOfBitsInBuffer != 0) {
      temp = GetBits(m_uNumOfBitsInBuffer);
#if 0
      temp = m_uNumOfBitsInBuffer;
      m_uNumOfBitsInBuffer = 0;
      m_chDecBuffer++;
      m_chDecBufferSize -= MIN(m_chDecBufferSize,8);
#endif
    } else {
      // if we are byte aligned, check for 0x7f value - this will indicate
      // we need to skip those bits
      uint8_t readval;
      readval = PeekBits(8);
      if (readval == 0x7f) {
	readval = GetBits(8);
      }
    }
    return (temp);
  }

void CBitstream::set_verbose(int verbose) { m_verbose = verbose; };


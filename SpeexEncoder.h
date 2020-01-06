#pragma once
#include <speex/speex.h>

class SpeexEncoder
{
public:
	SpeexEncoder();
	~SpeexEncoder();

	int Init(unsigned sampleRate, unsigned short channels, unsigned short bitDeepth);

	int InputRawData(const unsigned char* pcmData, size_t sz);

	void FlushEncoder();
private:
	SpeexBits m_bists;
	void* m_enc;
	unsigned int m_inputBytes;
};


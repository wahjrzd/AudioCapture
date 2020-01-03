#pragma once
#include <faac.h>
#include <string>

class AACEncoder
{
public:
	AACEncoder();
	~AACEncoder();

	int Init(unsigned sampleRate, unsigned short channels, unsigned short deepth);

	int InputRawData(const uint8_t* pcmData, size_t sz);

	void FlushEncoder();
private:
	unsigned char* m_outputBuffer;
	unsigned char* m_bufferPool;
	unsigned int m_offset = 0;
	unsigned int m_validSize = 0;
	faacEncHandle m_encHandle;
	unsigned long m_inputSamples;
	unsigned long m_maxOutputBytes;
	FILE* pf;
};


#include "pch.h"
#include "AACEncoder.h"
#include <iostream>

AACEncoder::AACEncoder() :m_encHandle(nullptr), m_outputBuffer(nullptr), m_bufferPool(nullptr)
{
	fopen_s(&pf, "1.aac", "wb");
}

AACEncoder::~AACEncoder()
{
	if (m_encHandle)
		faacEncClose(m_encHandle);

	delete m_outputBuffer;
	delete m_bufferPool;

	fclose(pf);
}

int AACEncoder::Init(unsigned sampleRate, unsigned short channels, unsigned short bitDeepth)
{
	m_encHandle = faacEncOpen(sampleRate, channels, &m_inputSamples, &m_maxOutputBytes);
	if (!m_encHandle)
		return 1;
	auto conf = faacEncGetCurrentConfiguration(m_encHandle);
	conf->aacObjectType = LOW;
	conf->inputFormat = FAAC_INPUT_16BIT;
	conf->useLfe = 0;
	conf->useTns = 0;
	conf->outputFormat = 1;
	conf->allowMidside = 0;
	conf->bitRate = sampleRate * channels * bitDeepth / 8;
	conf->bandWidth = 0;
	conf->shortctl = SHORTCTL_NORMAL;
	faacEncSetConfiguration(m_encHandle, conf);
	
	m_outputBuffer = new unsigned char[m_maxOutputBytes];
	m_bufferPool = new unsigned char[sampleRate*channels*bitDeepth / 8];
	m_inputBytes = m_inputSamples * bitDeepth / 8;
	return 0;
}

int AACEncoder::InputRawData(const uint8_t* pcmData, size_t sz)
{
	int ret = 0;
	memcpy(m_bufferPool + m_offset, pcmData, sz);
	m_validSize += sz + m_offset;
	
	auto quo = m_validSize / m_inputBytes;
	auto rem = m_validSize % m_inputBytes;

	for (size_t i = 0; i < quo; i++)
	{
		ret = faacEncEncode(m_encHandle, (int32_t*)(m_bufferPool + i * m_inputBytes),
			m_inputSamples, m_outputBuffer, m_maxOutputBytes);
		if (ret < 0)
		{
			std::clog << "faacEncEncode return" << ret << std::endl;
			break;
		}
		else if (ret == 0)
		{
			std::clog << "ret zero" << std::endl;
		}
		else
		{

			fwrite(m_outputBuffer, 1, ret, pf);
		}
	}

	m_validSize = rem;
	m_offset = rem;

	if (rem != 0)
		memmove(m_bufferPool, m_bufferPool + quo * m_inputBytes, rem);
	return 0;
}

void AACEncoder::FlushEncoder()
{
	int ret;
	while (true)
	{
		ret = faacEncEncode(m_encHandle, nullptr, 0, m_outputBuffer, m_maxOutputBytes);
		if (ret <= 0)
			break;
		else
		{

		}
	}
}
#include "pch.h"
#include "SpeexEncoder.h"

SpeexEncoder::SpeexEncoder()
{

}

SpeexEncoder::~SpeexEncoder()
{

}

int SpeexEncoder::Init(unsigned sampleRate, unsigned short channels, unsigned short bitDeepth)
{
	unsigned int frame_size;
	int quality = 10;
	SpeexBits bits;
	speex_bits_init(&m_bists);

	m_enc = speex_encoder_init(&speex_nb_mode);

	speex_encoder_ctl(m_enc, SPEEX_GET_FRAME_SIZE, &frame_size);
	speex_encoder_ctl(m_enc, SPEEX_SET_QUALITY, &quality);

	m_inputBytes = frame_size * bitDeepth / 8;
	return 0;
}

int SpeexEncoder::InputRawData(const unsigned char* pcmData, size_t sz)
{
	char cbits[200];
	speex_bits_reset(&m_bists);
	spx_int16_t input;
	speex_encode_int(m_enc, &input, &m_bists);
	auto cc = speex_bits_nbytes(&m_bists);
	auto xx = speex_bits_write(&m_bists, cbits, 200);
	return 0;
}

void SpeexEncoder::FlushEncoder()
{

}
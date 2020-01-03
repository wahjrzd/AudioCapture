#pragma once
#include <dsound.h>
#include <wrl.h>
#include <string>
#include <vector>
#include <future>
#include <queue>

class AACEncoder;

class AudioCapture
{
public:
	AudioCapture();
	~AudioCapture();

	int Init(DWORD sampleRate);

	int Start();

	void Stop();
private:
	static BOOL CALLBACK cb(LPGUID id, LPCWSTR desc, LPCWSTR mode, LPVOID context);

	unsigned wrapRun();

	unsigned wrapEncode();
private:
	Microsoft::WRL::ComPtr<IDirectSoundCapture8> m_soundCap;
	Microsoft::WRL::ComPtr<IDirectSoundCaptureBuffer8> m_soundCapBuffer;
	std::future<unsigned> m_capFunc;
	std::future<unsigned> m_encFunc;
private:
	DWORD m_sampleRate;
	DWORD m_bufferBytes;
	std::vector<LPCGUID> m_capGuids;
	HANDLE m_notifyEvents[3];
private:
	AACEncoder* m_aacEncoder;
	std::mutex m_queueLock;
	std::condition_variable m_queueCond;
	std::queue <std::basic_string<uint8_t>> m_rawQueue;
};


#include "pch.h"
#include "AudioCapture.h"
#include "CWaveFile.h"
#include <iostream>
#include "AACEncoder.h"

AudioCapture::AudioCapture() :m_aacEncoder(nullptr)
{
	for (size_t i = 0; i < 3; i++)
	{
		m_notifyEvents[i] = CreateEvent(NULL, TRUE, FALSE, NULL);
	}
}  

AudioCapture::~AudioCapture()
{
	for (size_t i = 0; i < 3; i++)
	{
		CloseHandle(m_notifyEvents[i]);
	}
	delete m_aacEncoder;
}

BOOL CALLBACK AudioCapture::cb(LPGUID id, LPCWSTR desc, LPCWSTR mode, LPVOID context)
{
	auto p = (AudioCapture*)context;
	p->m_capGuids.push_back(id);
	return TRUE;
}

int AudioCapture::Init(DWORD sampleRate)
{
	m_sampleRate = sampleRate;

	HRESULT hr;
	DirectSoundCaptureEnumerate(cb, this);

	if (m_capGuids.size() < 2)
		hr = DirectSoundCaptureCreate8(NULL, &m_soundCap, NULL);
	else
		hr = DirectSoundCaptureCreate8(m_capGuids[1], &m_soundCap, NULL);

	if (FAILED(hr))
	{
		std::clog << "DirectSoundCaptureCreate8 failed" << std::endl;
		return 1;
	}
	DSCCAPS cap = { sizeof(DSCCAPS) }; //e.g. WAVE_FORMAT_4S08
	m_soundCap->GetCaps(&cap);//获取设备能力

	Microsoft::WRL::ComPtr<IDirectSoundCaptureBuffer> tempBuffer;
	Microsoft::WRL::ComPtr<IDirectSoundNotify8> soundNotify;
	
	WAVEFORMATEX wfx;
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nChannels = 2;
	wfx.nSamplesPerSec = m_sampleRate;
	wfx.wBitsPerSample = 16;//8 or 16
	wfx.nBlockAlign = wfx.nChannels*wfx.wBitsPerSample / 8;
	wfx.nAvgBytesPerSec = wfx.nSamplesPerSec*wfx.nBlockAlign;
	wfx.cbSize = 0;

	DSCBUFFERDESC desc;
	DSCEFFECTDESC effectDesc;
	effectDesc.dwSize = sizeof(DSCEFFECTDESC);
	effectDesc.dwReserved1 = 0;
	effectDesc.dwReserved2 = 0;
	effectDesc.dwFlags = DSCFX_LOCSOFTWARE;
	effectDesc.guidDSCFXClass = GUID_DSCFX_CLASS_NS;//降噪
	effectDesc.guidDSCFXInstance = GUID_DSCFX_MS_NS;

	desc.dwSize = sizeof(DSCBUFFERDESC);
	desc.dwBufferBytes = wfx.nAvgBytesPerSec;
	desc.dwFlags = DSCBCAPS_CTRLFX;
	desc.dwReserved = 0; 
	desc.dwFXCount = 1;
	desc.lpwfxFormat = &wfx;
	desc.lpDSCFXDesc = &effectDesc;

	hr = m_soundCap->CreateCaptureBuffer(&desc, &tempBuffer, NULL);
	if (SUCCEEDED(hr))
	{
		hr = tempBuffer->QueryInterface(IID_IDirectSoundCaptureBuffer8, &m_soundCapBuffer);
		tempBuffer.Reset();
	}
	else
	{
		std::clog << "CreateCaptureBuffer failed" << std::endl;
		return 1;
	}
	DSCBCAPS dscCaps = { sizeof(DSCBCAPS) };
	m_soundCapBuffer->GetCaps(&dscCaps); 
	m_bufferBytes = dscCaps.dwBufferBytes;

	WAVEFORMATEX tempWfx;
	hr = m_soundCapBuffer->GetFormat(&tempWfx, sizeof(WAVEFORMATEX), NULL);

	hr = m_soundCapBuffer->QueryInterface(IID_IDirectSoundNotify8, &soundNotify);
	if (FAILED(hr))
	{
		std::clog << "create DirectSoundNotify failed" << std::endl;
		return 1;
	}

	DSBPOSITIONNOTIFY dsbNotify[3];
	dsbNotify[0].dwOffset = (tempWfx.nAvgBytesPerSec / 2) - 1;
	dsbNotify[0].hEventNotify = m_notifyEvents[0];

	dsbNotify[1].dwOffset = tempWfx.nAvgBytesPerSec - 1;
	dsbNotify[1].hEventNotify = m_notifyEvents[1];

	dsbNotify[2].dwOffset = DSBPN_OFFSETSTOP;
	dsbNotify[2].hEventNotify = m_notifyEvents[2];
	hr = soundNotify->SetNotificationPositions(3, dsbNotify);

	m_aacEncoder = new AACEncoder();
	return 0;
}

int AudioCapture::Start()
{
	if (!m_capFunc.valid())
		m_capFunc = std::async(std::launch::async, &AudioCapture::wrapRun, this);

	if (!m_encFunc.valid())
		m_encFunc = std::async(std::launch::async, &AudioCapture::wrapEncode, this);

	return 0;
}

unsigned AudioCapture::wrapRun()
{
	HRESULT hr;

	VOID* pbCaptureData = NULL;
	DWORD dwCaptureLength;
	VOID* pbCaptureData2 = NULL;
	DWORD dwCaptureLength2;
	DWORD dwReadPos;
	DWORD nextOffset = 0;
	UINT writeBytes;
	LONG lLockSize;
	DWORD dwRet;

	CWaveFile g_pWaveFile;
	WAVEFORMATEX  wfxInput;

	ZeroMemory(&wfxInput, sizeof(wfxInput));
	wfxInput.wFormatTag = WAVE_FORMAT_PCM;
	wfxInput.nSamplesPerSec = m_sampleRate;
	wfxInput.wBitsPerSample = 16;
	wfxInput.nChannels = 2;
	wfxInput.nBlockAlign = wfxInput.nChannels * (wfxInput.wBitsPerSample / 8);
	wfxInput.nAvgBytesPerSec = wfxInput.nBlockAlign * wfxInput.nSamplesPerSec;

	WCHAR fileName[] = L"test.wav";
	hr = g_pWaveFile.Open(fileName, &wfxInput, WAVEFILE_WRITE);
	if (FAILED(hr))
	{
		std::clog << "wave file open failed" << std::endl;
		return 1;
	}
	std::clog << "start capture audio data..." << std::endl;

	hr = m_soundCapBuffer->Start(DSCBSTART_LOOPING);
	while (true)
	{
		dwRet = WaitForMultipleObjects(3, m_notifyEvents, FALSE, INFINITE);
		auto index = dwRet - WAIT_OBJECT_0;
		if (index == 2)
			break;
		else
			ResetEvent(m_notifyEvents[index]);

		hr = m_soundCapBuffer->GetCurrentPosition(NULL, &dwReadPos);
		if (FAILED(hr))
		{
			std::clog << "GetCurrentPosition failed" << std::endl;
			break;
		}

		lLockSize = dwReadPos - nextOffset;
		if (lLockSize < 0)
			lLockSize += m_bufferBytes;
		else if (lLockSize == 0)
			continue;

		hr = m_soundCapBuffer->Lock(nextOffset, lLockSize, &pbCaptureData, &dwCaptureLength,
			&pbCaptureData2, &dwCaptureLength2, 0);
		if (FAILED(hr))
		{
			std::clog << "lock buffer failed" << std::endl;
			break;
		}
		else
		{
			std::lock_guard<std::mutex> lck(m_queueLock);
			m_rawQueue.push(std::basic_string<uint8_t>((uint8_t*)pbCaptureData, dwCaptureLength));
			//g_pWaveFile.Write(dwCaptureLength, (BYTE*)pbCaptureData, &writeBytes);
			if (pbCaptureData2)
			{
				m_rawQueue.push(std::basic_string<uint8_t>((uint8_t*)pbCaptureData2, dwCaptureLength2));
				//g_pWaveFile.Write(dwCaptureLength2, (BYTE*)pbCaptureData2, &writeBytes);
			}
		}
		m_soundCapBuffer->Unlock(pbCaptureData, dwCaptureLength, pbCaptureData2, dwCaptureLength2);

		nextOffset += dwCaptureLength;
		nextOffset %= m_bufferBytes;
		nextOffset += dwCaptureLength2;
		nextOffset %= m_bufferBytes;
	}
	g_pWaveFile.Close();
	return 0;
}

unsigned AudioCapture::wrapEncode()
{
	while (true)
	{
		std::unique_lock<std::mutex> lck(m_queueLock, std::adopt_lock);
		if (m_rawQueue.empty())
			m_queueCond.wait(lck);
		if (m_rawQueue.empty())
			break;
		auto obj = m_rawQueue.front();
		m_rawQueue.pop();
		lck.unlock();
		
		m_aacEncoder->InputRawData(obj.c_str(), obj.size());
	}
	return 0;
}

void AudioCapture::Stop()
{
	SetEvent(m_notifyEvents[2]);
	m_soundCapBuffer->Stop();

	m_queueCond.notify_one();

	if (m_capFunc.valid())
		m_capFunc.wait();

	if (m_encFunc.valid())
		m_encFunc.wait();
}
#include "pch.h"
#include <iostream>
#include "AudioCapture.h"

#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dsound.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "libfaac.lib")
#pragma comment(lib, "libspeex.lib")

int main()
{
#if defined(WIN32)&&defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	
	CoInitialize(NULL);
	{
		AudioCapture ac;
		ac.Init(24000);

		ac.Start();

		std::string line;
		std::getline(std::cin, line);

		ac.Stop();
	}
	CoUninitialize();
	return 0;
}

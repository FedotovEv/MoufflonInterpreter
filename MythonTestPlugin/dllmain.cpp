
// dllmain.cpp : Определяет точку входа для приложения DLL.
#include "pch.h"

#if defined (_WIN64) || defined(_WIN32)
    // Вариант точки входа в динамическую библиотеку для Windows.
    BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
    {
        switch (ul_reason_for_call)
        {
        case DLL_PROCESS_ATTACH:
            [[fallthrough]];
        case DLL_THREAD_ATTACH:
            [[fallthrough]];
        case DLL_THREAD_DETACH:
            [[fallthrough]];
        case DLL_PROCESS_DETACH:
            break;
        }
        return TRUE;
    }
#elif defined(__unix__) || defined(__linux__) || defined(__USE_POSIX)
    // Вариант точки входа в динамическую библиотеку для чего-то линуксоподобного.

#endif
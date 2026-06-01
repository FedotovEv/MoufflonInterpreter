
// dllmain.cpp : Определяет точку входа для приложения DLL.
#include "pch.h"

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

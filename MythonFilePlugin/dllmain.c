
// Главный файл динамической библиотеки - определяет точку входа для приложения DLL.

#include "file_plugin.h"

#if defined (_WIN64) || defined(_WIN32)
    // Вариант оформления точки входа динамической библиотеки для ОС Windows.
    #include "framework.h"
    BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
    {
        switch (ul_reason_for_call)
        {
        case DLL_PROCESS_ATTACH:
            break;
        case DLL_THREAD_ATTACH:
            break;
        case DLL_THREAD_DETACH:
            break;
        case DLL_PROCESS_DETACH:
            ClearPluginStatuses();
            break;
        }
        return TRUE;
    }
#elif defined(__unix__) || defined(__linux__) || defined(__USE_POSIX)
    // Вариант точки входа в динамическую библиотеку для чего-то линуксоподобного.

#endif

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <variant>

#if defined (_WIN64) || defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #undef GetClassName
    #undef GetClassNameA
    #undef GetClassNameW
#elif defined(__unix__) || defined(__linux__) || defined(__USE_POSIX)
    #include <dlfcn.h>
#endif

#if defined (_WIN64) || defined(_WIN32)
    #ifdef MYTHLON_INTERPRETER_MODULE
        #define MYTHLON_INTERPRETER_PUBLIC __declspec(dllimport)
    #else
        #define MYTHLON_INTERPRETER_PUBLIC __declspec(dllexport)
    #endif
#else
    #define MYTHLON_INTERPRETER_PUBLIC
#endif

#define ZERO_TOLERANCE 0.000000001

namespace runtime
{
    using NumberValue = std::variant<int, double>;
    enum class LinkCallReason
    {
        CALL_REASON_UNKNOWN = 0,
        CALL_REASON_READ_FIELD,
        CALL_REASON_WRITE_FIELD,
        CALL_REASON_CALL_METHOD
    };

    using LinkageReturn = std::variant<int, double, std::string>;
    using LinkageFunction = std::function<LinkageReturn(LinkCallReason, const std::string&,
        const std::vector<std::string>&)>;

    struct ProgramCommandDescriptor
    {
        int module_id = -1;
        int module_string_number = -1;

        bool operator==(const ProgramCommandDescriptor& other)
        {
            return module_id == other.module_id &&
                   module_string_number == other.module_string_number;
        }
    
        bool operator!=(const ProgramCommandDescriptor& other)
        {
            return module_id != other.module_id ||
                   module_string_number != other.module_string_number;
        }
    };
} // namespace return

#pragma once

#include "declares.h"
#include <memory>
#include <stdexcept>

#include "throw_messages.h"

namespace runtime
{
    class Executable;
}

// Ќа данный момент поддерживаютс€ два способа загрузки внешних двоичных расширений ("втыкал") -
// - из файла стандартной раздел€емой библиотеки (.DLL или .SO, в зависимости от операционной системы),
// а также непосредственно из пам€ти, если весь нужный объектно-процедурный комплекс уже сформирован
// внешней системой, котора€ подключила и использует этот интерпретатор.
// ≈сли LoadLibraryDefine содержит string - втыкало загружаетс€ из раздел€емой библиотеки, им€ которой
// совпадает с этой строкой. Ёто им€ без дальнейшей обработки будет передано службе динамической загрузки
// операционной системы (LoadLibraryW дл€ виндовс или dlopen дл€ линукса).
// ≈сли же LoadLibraryDefine содержит тип InternalObjectCreatorList - втыкало подключаетс€, использу€
// пары из имени класса и указател€ на "фабричный метод", которые и содержатс€ в массиве LoadLibraryDefine.
// ¬ случае, когда LoadLibraryDefine содержит monostate, втыкало не загружаетс€.

using FuncInternalObjectCreator = std::unique_ptr<runtime::Executable>
                            (std::vector<std::unique_ptr<runtime::Executable>>);
using InternalObjectCreator = std::function<FuncInternalObjectCreator>;
using InternalObjectCreatorList = std::vector<std::pair<std::string, InternalObjectCreator>>;
using LoadLibraryDefine = std::variant<std::monostate, std::string, InternalObjectCreatorList>;

namespace parse
{
    class Lexer;
    class ParseContext
    {
    public:
        ParseContext() : is_auto_deallocate_(false)
        {}
        ParseContext(bool is_auto_deallocate) : is_auto_deallocate_(is_auto_deallocate)
        {}
        virtual ~ParseContext();
        virtual LoadLibraryDefine GetLoadLibraryDesc(const std::string& library_name) const = 0;
        #if defined (_WIN64) || defined(_WIN32)
            void AddDLLEntry(HMODULE hAddonDll)
            {
                dll_list_.push_back(hAddonDll);
            }
        #elif defined(__unix__) || defined(__linux__) || defined(__USE_POSIX)
            void AddDLLEntry(void* hAddonDll)
            {
                dll_list_.push_back(hAddonDll);
            }
        #endif
        void DeallocateGlobalResources();

    private:
        bool is_auto_deallocate_ = false;
        #if defined (_WIN64) || defined(_WIN32)
            std::vector<HMODULE> dll_list_;
        #elif defined(__unix__) || defined(__linux__) || defined(__USE_POSIX)
            std::vector<void*> dll_list_;
        #endif
    };

    class TrivialParseContext : public ParseContext
    {
    public:
        TrivialParseContext() : ParseContext()
        {}
        TrivialParseContext(bool is_auto_deallocate) : ParseContext(is_auto_deallocate)
        {}        
        LoadLibraryDefine GetLoadLibraryDesc(const std::string& library_name) const override;
    };
}

struct ParseError : std::runtime_error
{
    using std::runtime_error::runtime_error;
    ParseError(runtime::ThrowMessageNumber throw_message_number);
};

std::unique_ptr<runtime::Executable> ParseProgram(parse::Lexer& lexer);
std::unique_ptr<runtime::Executable> ParseProgram(parse::Lexer& lexer, parse::ParseContext& parse_context);

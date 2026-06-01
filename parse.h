#pragma once

#include "declares.h"
#include <memory>
#include <stdexcept>

#include "throw_messages.h"

namespace runtime
{
    class Executable;
}

//   На данный момент поддерживаются два способа загрузки внешних двоичных расширений ("втыкал") -
// - из файла стандартной разделяемой библиотеки (.DLL или .SO, в зависимости от операционной системы),
// а также непосредственно из памяти, если весь нужный объектно-процедурный комплекс уже сформирован
// внешней системой, которая подключила и использует этот интерпретатор.
//   Если LoadLibraryDefine содержит string - втыкало загружается из разделяемой библиотеки, имя которой
// совпадает с этой строкой. Это имя без дальнейшей обработки будет передано службе динамической загрузки
// операционной системы (LoadLibraryW для виндовс или dlopen для линукса).
//   Если же LoadLibraryDefine содержит тип PluginGetInfoFunc (указатель на информирующую функцию) - втыкала
// подключается, используя данные, которые эта функция предоставляет по соответствующим запросам к ней.
//   В случае, когда LoadLibraryDefine содержит monostate, втыкало не загружается.

// Функциональный тип, определяющий фабричный метод встроенного класса, а также оболочка std::function для него.
using FuncInternalObjectCreator = std::unique_ptr<runtime::Executable>(std::vector<std::unique_ptr<runtime::Executable>>);
using InternalObjectCreator = std::function<FuncInternalObjectCreator>;
// 
using LoadLibraryDefine = std::variant<std::monostate, std::string, PluginGetInfoFunc>;

// Ожидаемое имя головной функции динамически загружаемой библиотеки со втыкалами.
constexpr char GET_PLUGINS_INFO_FUNCTION[] = PLUGINS_GET_INFO_FUNCTION;

namespace ast
{
    struct MethodDefiner
    { // "Внутренняя" (то есть применяемая внутри функций и классов самого интерпретатора Муфлона) структура описания некоторого метода,
      // предоставляемого классом-втыкалой.
        std::string name;               // Имя метода.
        size_t arg_count_min = 0;       // Минимально допустимое количество его параметров.
        size_t arg_count_max = 0;       // Максимально допустимое количество его параметров.
        // Если метод имеет фиксированное и однозначно определённое количество параметров, можно выполнить контроль соответствия их
        // фактического типа требуемому.
        // Режим проверки допустимости фактических параметров метода.
        MethodParamCheckMode check_mode = MethodParamCheckMode::PARAM_CHECK_NONE;
        // Список, указывающий допустимый тип для очередного фактического параметра.
        std::vector<MethodParamType> param_types;
    };

    // Структура описания отдельной загруженной втыкалы.
    struct PluginDescData
    {
        PluginGetInfoFunc info_func = nullptr;      // Указатель на информирующую функцию втыкалы.
        PluginCallMethodFunc call_func = nullptr;   // Указатель на её "вызывающую" функцию.
        std::unordered_multimap<std::string, ast::MethodDefiner> methods; // Множество характеристик (имён, и.т.д.) доступных методов класса втыкалы.    
    };
} // namespace ast

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
    ParseError(ThrowMessageNumber throw_message_number);
};

std::unique_ptr<runtime::Executable> ParseProgram(parse::Lexer& lexer);
std::unique_ptr<runtime::Executable> ParseProgram(parse::Lexer& lexer, parse::ParseContext& parse_context);

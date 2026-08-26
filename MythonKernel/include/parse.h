#pragma once

#include "declares.h"
#include <memory>
#include <stdexcept>

#include "throw_messages.h"
#include "encodings.h"

namespace runtime
{
    class Executable;
    class ObjectHolder;
    class Context;
    // Closure - таблица символов, связывающая имя объекта с его значением.
    using Closure = std::unordered_map<std::string, ObjectHolder>;
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
//   В случае, когда LoadLibraryDefine содержит monostate, втыкала не загружается.
using LoadLibraryDefine = std::variant<std::monostate, std::string, PluginGetInfoFunc>; // Тип определителя загруженной (подключённой) втыкалы.

// Ожидаемое имя головной функции динамически загружаемой библиотеки со втыкалами.
constexpr char GET_PLUGINS_INFO_FUNCTION[] = PLUGINS_GET_INFO_FUNCTION;
// Максимальная длина имён функций и методов, которые могут использоваться при взаимодействии с модулями втыкал.
constexpr size_t MAX_PLUGIN_NAMES_LEN = 1024;

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
        virtual ~ParseContext() = default;
        virtual LoadLibraryDefine GetLoadLibraryDesc(const std::string& library_name) const = 0;
    };

    class TrivialParseContext : public ParseContext
    {
    public:
        TrivialParseContext() : ParseContext()
        {}
        LoadLibraryDefine GetLoadLibraryDesc(const std::string& library_name) const override;
    };

    // Класс для ведения идентификации создаваемых типов (классов различного вида), а также выделения им уникальных числовых
    // идентификаторов.
    class TypeIdentificator
    {
    public:
        static int GetNewTypeId()
        {
            return current_type_id++;
        }

    private:
        static int current_type_id;     // Поле для отслеживания текущего выделяемого идента для вновь создаваемого класса.
    };
}

struct ParseError : std::runtime_error
{
    using std::runtime_error::runtime_error;
    ParseError(ThrowMessageNumber throw_message_number);
};

// Функциональный тип, определяющий фабричный метод встроенного класса, а также оболочка std::function для него.
using FuncInternalObjectCreator = std::unique_ptr<runtime::Executable>(std::vector<std::unique_ptr<runtime::Executable>>);
struct InternalObjectCreator
{
    int my_id = parse::TypeIdentificator::GetNewTypeId();
    std::function<FuncInternalObjectCreator> creator;
};

std::unique_ptr<runtime::Executable> ParseProgram(parse::Lexer& lexer);
std::unique_ptr<runtime::Executable> ParseProgram(parse::Lexer& lexer, parse::ParseContext& parse_context);

/*
// Структура для компактного хранения всех активов, необходимых для разбора, анализа и последующего исполнения МУФЛОН-программы.
// Структура принимает в единоличное владение все назначенные ей объекты.
struct CplxParsedProgram
{
    CplxParsedProgram();
    CplxParsedProgram(const CplxParsedProgram& other) = delete;
    CplxParsedProgram(CplxParsedProgram&& other) = default;
    ~CplxParsedProgram();

    CplxParsedProgram& operator=(const CplxParsedProgram& other) = delete;
    CplxParsedProgram& operator=(CplxParsedProgram&& other) = default;

    std::unique_ptr<parse::Lexer> lexer;                    // Грамматический разборщик.
    std::unique_ptr<runtime::Executable> program;           // Указатель на корневой узел АСТ программы после её разбора и анализа.
    std::unique_ptr<parse::ParseContext> parse_context;     // Контекст синтаксического анализа.
    std::unique_ptr<runtime::Closure> closure;              // Таблица символов для исполнения программы.
    std::unique_ptr<runtime::Context> context;              // Контекст исполнения.

    CplxParsedProgram& SetLexer(parse::Lexer&& p_lexer);
    CplxParsedProgram& SetClosure(runtime::Closure&& p_closure);

    // Возвращает "ИСТИНУ", если объект содержит разобранную программу, готовую к исполнению.
    bool IsParsed() const
    {
        return bool(program);
    }

    // Классы parse::ParseContext и runtime::Context абстрактные и чисто виртуальные.
    template<typename ParseContextT>
    CplxParsedProgram& SetParseContext(ParseContextT&& p_parse_context)
    {
        parse_context = std::make_unique<ParseContextT>(std::forward<ParseContextT>(p_parse_context));
        return *this;
    }

    template<typename ContextT>
    CplxParsedProgram& SetContext(ContextT&& p_context)
    {
        context = std::make_unique<ContextT>(std::forward<ContextT>(p_context));
        return *this;
    }
};

// Функция синтаксического анализа программы, исходный код которой предоставляется через lexer.
void ParseProgram(CplxParsedProgram& cplx_program);
// Функция исполнения разобранной и проанализированной программы, для которой построено АСД.
runtime::ObjectHolder ExecuteProgram(CplxParsedProgram& program);
*/


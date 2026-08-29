#pragma once

#include <string>
#include <vector>
#include <functional>
#include <variant>
#include <iostream>

// ВНИМАНИЕ: Для блокировки механизмов многопоточности и потокобезопасности некоторых структур комплекса (скажем, в случае, если
// потоки и связанная с ними инфраструктура не поддерживаются используемым компилятором), следует определить макрос MYTHON_UNITHREAD.

#ifdef MYTHON_UNITHREAD
    #warning "Используется однопоточный вариант интерпретатора МУФЛОНА"
#endif

#if defined (_WIN64) || defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #undef GetClassName
    #undef GetClassNameA
    #undef GetClassNameW
#elif defined(__unix__) || defined(__linux__) || defined(__USE_POSIX)
    #include <dlfcn.h>
#endif

#define ZERO_TOLERANCE 0.000000001

const std::string INIT_METHOD = "__init__";
const std::string DESTROY_METHOD = "__destroy__";
const std::string ADD_METHOD = "__add__";
const std::string EXTERNAL_LINK_CLASS_NAME = "__external";
const std::string EQUAL_CMP_METHOD = "__eq__";
const std::string LESS_CMP_METHOD = "__lt__";
const std::string STR_FUNCTION_METHOD = "__str__";
const std::string FUNCTOR_CALL_METHOD = "__run__";
const std::string COROUTINE_STATUS_VAR = "__coro__";
const std::string SELF_FIELD_NAME = "self";
const std::string BREAKPOINT_INFO_FIELD_NAME = "__breakpoint__";
// Имена стандартных встроенных фиксированнных (неизменяемых и ненаследуемых) классов.
const std::string ARRAY_CLASS_NAME = "array";
const std::string MAP_CLASS_NAME = "map";
const std::string MATH_CLASS_NAME = "math";
const std::string STRINGOPS_CLASS_NAME = "string_ops";
// Имена стандартных классов ошибок (также ненаследуемых).
const std::string COMMON_ERROR_CLASS_NAME = "CommonError";
const std::string SYSTEM_ERROR_CLASS_NAME = "SystemError";
const std::string ERROR_DIVISION_BY_ZERO_CLASS_NAME = "ErrorDivisionByZero";
const std::string OVERFLOW_ERROR_CLASS_NAME = "OverflowError";
const std::string DOMAIN_ERROR_CLASS_NAME = "DomainError";
const std::string ERROR_PARAMS_INCONSISTENCY_CLASS_NAME= "ErrorParamsInconsistency";
const std::string SYNTAX_ERROR_CLASS_NAME = "SyntaxError";
const std::string MODULE_ERROR_CLASS_NAME = "ModuleError";
const std::string LOGIC_ERROR_CLASS_NAME = "LogicError";
const std::string REFERENCE_ERROR_CLASS_NAME = "ReferenceError";
// Константы имени прототипа класса-ждуна и его стандартных специальных методов.
const std::string AWAITABLE_CLASS_NAME = "Awaitable";
const std::string AWAITABLE_SUSPEND_METHOD = "AwaitSuspend";
const std::string AWAITABLE_RESUME_METHOD = "AwaitResume";
// Имя класса типового отпечатка (типовых характеристик, TypeTraits).
const std::string TYPE_TRAITS_CLASS_NAME = "TypeTraits";
// Префиксы строковых литералов с указанием некоторых особенностей.
const std::string UTF_8_STRING_LITERAL = "u8";       // Строковый литерал в UTF-8-представлении.
const std::string NARROW_STRING_LITERAL = "n";       // "Узкий" (однобайтовый) строковый литерал без указания кодировки.
const std::string NAMED_ENC_STRING_LITERAL = "enc_"; // Строковый литерал с указанием конкретной его кодировки по имени.
// Иденты встроенных типов.
constexpr int INVALID_TYPE_IDENT = -1;      // Несостоятельный идент, не соответствующий какому-либо реально существующему типу.
constexpr int NONE_IDENT = 0;               // Идент типа пустого выражения None.
constexpr int BOOL_IDENT = 1;               // Идент логического типа.
constexpr int NUMERIC_IDENT = 2;            // Идент числового типа.
constexpr int STRING_IDENT = 3;             // Идент строкового типа.
constexpr int CLASS_AREA_IDENTS = 1000;     // Начало области классовых идентов (идентификатор первого класса, определённого в
                                            // программе помимо базовых типов).
constexpr size_t COLLATE_SIZE = 256U;       // Длина правильной строки относительных весов символов.

enum EncodingCharClasses
{
    CHAR_CLASS_NOTHING = 0,     // Не относится ни к какому особому типу.
    CHAR_CLASS_LETTER = 1,      // Буква.
    CHAR_CLASS_DIGIT = 2,       // Десятичная цифра.
    CHAR_CLASS_XDIGIT = 4,      // Шестнадцатиричная цифра.
    CHAR_CLASS_CONTROL = 8,     // Управляющий символ.
    CHAR_CLASS_SPACE = 16,      // Пробел.
    CHAR_CLASS_BLANK = 32,      // Пробельные разделители слов.
    CHAR_CLASS_GRAPHIC = 64,    // Знаки с видимыми очертаниями.
    CHAR_CLASS_PRINT = 128,     // Печатаемые знаки.
    CHAR_CLASS_PUNCT = 256      // Знаки пунктуации.
};

// Структура, содержащая информацию о некоторой однобайтовой кодировке, необходимую для выполнения ряда операций над МУФЛОН-строками.
struct SingleByteEncodingDesc
{
    std::string name;   // Имя кодировки (не может быть пустым).
    // Таблица спаривания символов верхнего и нижнего регистра. Первый член пары - символ верхнего регистра, второй член - соответствующий
    // ему символ регистра нижнего.
    std::vector<std::pair<char, char>> upcase_table;
    // Строка "весов" символов, обеспечивающая их кодировочно-зависимое лексикографическое сравнение. Всегда содержит 256 элементов,
    // каждый из которых указывает относительный "вес" некоторого символа в данной кодировке, однобайтовый код которого равен порядковому
    // индексу соответствующего элемента в данной строке. При сравнении каждой пары символов символ с меньшим весом считается меньшим ("всплывает").
    std::string collate;
    // Массив соответствия однобайтового кода символа в данной кодировке и его многобайтового кода в кодировке UNICODE. Также всегда
    // содержит 256 элементов.
    std::vector<uint32_t> to_utf8;
    // Массив классификации отдельных символов в данной кодировке путём отнесения их к одному или сразу нескольким из некоторых определённых
    // символьных классов. Также всегда состоит из 256 элементов, по одному на соответствующий код символа.
    std::vector<EncodingCharClasses> char_classifier;

    bool IsCollateValid() const
    {
        return collate.size() == COLLATE_SIZE;
    }

    bool IsClassifierValid() const
    {
        return char_classifier.size() == COLLATE_SIZE;  // По одному элементу на каждый возможный однобайтовый код.
    }

    // Функции-члены классификации символа test_char, то есть проверки его принадлежности одному или нескольким символьным классам.
    bool IsAlpha(int test_char) const;
    bool IsAlNum(int test_char) const;
    bool IsDigit(int test_char) const;
    bool IsXDigit(int test_char) const;
    bool IsLower(int test_char) const;
    bool IsUpper(int test_char) const;
    bool IsCntrl(int test_char) const;
    bool IsGraph(int test_char) const;
    bool IsSpace(int test_char) const;
    bool IsBlank(int test_char) const;
    bool IsPrint(int test_char) const;
    bool IsPunct(int test_char) const;
};

struct UTF8Map
{
    std::vector<size_t> begin_map;   // Положение начал многобайтовых кодов UTF-8-символов в теле однобайтовой строки.
    size_t last_symbol_size = 0;     // Длина последнего кода в строке.

    void Clear()
    {
        begin_map.clear();
        last_symbol_size = 0;
    }

    bool IsValid() const
    {
        return !begin_map.empty();
    }

    // Возврат действительной длины строки в символах.
    size_t SymbolSizeOf() const
    {
        return begin_map.size();
    }

    // Функция-член возвращает байтовую позицию сразу за концом корректной UTF-8-строки.
    size_t BytePosAfterEnd() const;
    // Возвращает байтовую позицию символа с индексом symb_index.
    size_t SymbolBytePos(size_t symb_index) const;
    // Расчёт байтовой длины (длины в байтах) кода символа с индексом symb_index.
    size_t SymbolByteSize(size_t symb_index) const;
};

// Константы, связанные с обработкой кодировки различных строковых констант, встречающихся в МУФЛОН-программе.
// Константа для указания кодировки, находящейся вне общего хранилища кодировочных данных (массива encodings_data).
constexpr const int NON_INDEXED_ENCODING_ID = -1;
// Константы для строк без назначенной кодировки (режим по умолчанию).
// Указатель, отключающий кодировочные механизмы для некоторой строки (строка обрабатывается "как есть", то есть как массив байтов).
constexpr const SingleByteEncodingDesc* NO_ENCODING = nullptr;
// Числовой идентификатор отсутствия кодировки.
constexpr const int NO_ENCODING_ID = 0;
// Зарезервированное имя для сброса кодировки (отключения кодировочных механизмов при обработке строки).
const std::string NO_ENCODING_NAME = "NoEncoding";
// Константы для многобайтовой кодировки UTF-8.
// Указатель, обозначающий применение многобайтовой кодировки UTF-8 для некоторой строки.
const SingleByteEncodingDesc * const UTF_8_ENCODING = reinterpret_cast<const SingleByteEncodingDesc*>(intptr_t(-1));
// Числовой идентификатор кодировки UTF-8.
constexpr int UTF_8_ENCODING_ID = INT_MAX;
// Зарезервированное имя для кодировки UTF8.
const std::string UTF_8_ENCODING_NAME = "UTF-8";
constexpr size_t MAX_UNICODE_LENGTH = 6;	// Максимально допустимая поддерживаемая длина UTF-8-кода.

namespace runtime
{
    using NumberValue = std::variant<int, double>;
    enum class LinkCallReason
    { // Коды типов обращений к элементам объектов класса внешней связи EXTERNAL_LINK_CLASS_NAME (т. е. "__external").
        CALL_REASON_UNKNOWN = 0,
        CALL_REASON_READ_FIELD,         // Выполняется чтение поля объекта этого класса.
        CALL_REASON_WRITE_FIELD,        // Выполняется запись поля такого объекта.
        CALL_REASON_DELETE_FIELD,       // Удаляется какое-либо поле такого объекта (над ним исполняется оператор del).
        CALL_REASON_FIELD_IS_VISIBLE,   // Запрос видимости какого-то поля объекта.
        CALL_REASON_CALL_METHOD         // Производится вызов некоторого метода подобного объекта.
    };

    using LinkageValue = std::variant<std::monostate, bool, int, double, std::string>;
    using LinkageFunction = std::function<LinkageValue(LinkCallReason, const std::string&,
                                                       const std::vector<LinkageValue>&)>;

    struct ProgramCommandDescriptor
    {
        int module_id = -1;
        int module_string_number = 0;

        bool operator==(const ProgramCommandDescriptor& other) const
        {
            return module_id == other.module_id &&
                   module_string_number == other.module_string_number;
        }
    
        bool operator!=(const ProgramCommandDescriptor& other) const
        {
            return module_id != other.module_id ||
                   module_string_number != other.module_string_number;
        }

        bool IsValid() const
        {
            return module_string_number >= 0;
        }
    };
    // Зарезервированное некорректное (невалидное) значение позиционного описателя программной инструкции.
    constexpr ProgramCommandDescriptor DUMB_PROG_POS{.module_id = -1, .module_string_number = -1};

    std::string CommandDescToString(const ProgramCommandDescriptor& command_desc, int module_id_width = -1, int module_string_number_width = -1);
    std::ostream& operator<<(std::ostream& ostr, const ProgramCommandDescriptor& command_desc);

    enum class CoroutineSuspendType
    {
        SUSPEND_POINT_UNKNOWN = 0,
        SUSPEND_POINT_CO_YIELD,
        SUSPEND_POINT_CO_YIELD_REF,
        SUSPEND_POINT_CO_AWAIT
    };
} // namespace runtime

// Функция генерации декорированного имени метода либо функции, содержащего также элемент, кодирующий количество аргументов в нём.
std::string MangleMethodFunctionName(const std::string& method_func_name, size_t arg_count);
// Функция разделяет калечное имя метода или функции на его компоненты - само имя и количество аргументов процедуры.
std::pair<std::string, size_t> DemangleMethodFunctionName(const std::string& mangled_method_func_name);

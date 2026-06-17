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

#define ZERO_TOLERANCE 0.000000001

const std::string INIT_METHOD = "__init__";
const std::string DESTROY_METHOD = "__destroy__";
const std::string ADD_METHOD = "__add__";
const std::string EXTERNAL_LINK_CLASS_NAME = "__external";
const std::string EQUAL_CMP_METHOD = "__eq__";
const std::string LESS_CMP_METHOD = "__lt__";
const std::string STR_FUNCTION_METHOD = "__str__";
const std::string COROUTINE_STATUS_VAR = "__coro__";
const std::string SELF_FIELD_NAME = "self";
// Имена стандартных встроенных фиксированнных (неизменяемых и ненаследуемых) классов.
const std::string ARRAY_CLASS_NAME = "array";
const std::string MAP_CLASS_NAME = "map";
const std::string MATH_CLASS_NAME = "math";
const std::string STRINGOPS_CLASS_NAME = "string_ops";
// Имена стандартных классов ошибок (также ненаследуемых).
const std::string COMMON_ERROR_CLASS_NAME = "CommonError";
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
// Иденты встроенных типов.
constexpr int INVALID_TYPE_IDENT = -1;      // Несостоятельный идент, не соответствующий какому-либо реально существующему типу.
constexpr int NONE_IDENT = 0;               // Идент типа пустого выражения None.
constexpr int BOOL_IDENT = 1;               // Идент логического типа.
constexpr int NUMERIC_IDENT = 2;            // Идент числового типа.
constexpr int STRING_IDENT = 3;             // Идент строкового типа.
constexpr int CLASS_AREA_IDENTS = 1000;     // Начало области классовых идентов (идентификатор первого класса, определённого в
                                            // программе помимо базовых типов).

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
    };

    enum class CoroutineSuspendType
    {
        SUSPEND_POINT_UNKNOWN = 0,
        SUSPEND_POINT_CO_YIELD,
        SUSPEND_POINT_CO_YIELD_REF,
        SUSPEND_POINT_CO_AWAIT
    };
} // namespace runtime

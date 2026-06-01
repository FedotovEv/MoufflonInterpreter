#pragma once

// Кроме того, тут находятся некоторые вспомогательные функции и прочий синтаксический сахар, облегчающий использование втыкал другими
// частями комплекса.

#include <cstdint>

#if defined (_WIN64) || defined(_WIN32)
    #define MYTHLON_KERNEL_EXPORT __declspec(dllexport)
    #define MYTHLON_PLUGIN_IMPORT __declspec(dllimport)
#else
    #define MYTHLON_KERNEL_EXPORT
    #define MYTHLON_PLUGIN_IMPORT
#endif

#ifdef MYTHLON_PLUGIN
    // Заголовок включается в состав Муфлон-втыкалы.
    #define HELPERS_EXPORT_IMPORT MYTHLON_PLUGIN_IMPORT
#else
    // Заголовок включается в состав ядра интерпретатора Муфлона.
    #define HELPERS_EXPORT_IMPORT MYTHLON_KERNEL_EXPORT
#endif

// Перечисление типов значений, которые могут быть переданы методам втыкал, а также приняты от них как результаты их работы.
enum ObjectTypes : uint32_t
{
    OBJECT_TYPE_UNKNOWN = 0,
    OBJECT_TYPE_NONE = 1,           // Пустой параметр (и, соответственно, контейнер) None.
    OBJECT_TYPE_LOGICAL = 2,        // Логическое значение bool.
    OBJECT_TYPE_SYMBOL = 3,         // Одиночный символ char.
    OBJECT_TYPE_INTEGER = 4,        // Целочисленный параметр int32_t.
    OBJECT_TYPE_DOUBLE = 5,         // Число с плавающей точкой double.
    OBJECT_TYPE_STRING = 6,         // Символьная строка std::string.
    OBJECT_TYPE_OTHER = 0x7fffffff  // Значение, указывающее на какой-либо иной (прямо неподдерживаемый) тип данных, хранящийся в контейнере.
};

// Коды ошибок, возникающих при работе со втыкалами. Эти коды возвращаются функциями типа GetPluginInfo, а также всеми функциями, экспортируемыми
// ядром интерпретатора (PluginSetRuntimeError(), PluginSetResultValue(), и.т.д.) для нужд методов выткал. Все такие коды в обязательном порядке
// имеют величину < 0 (значения >= 0 служат для указания на нормальное завершение запроса и содержат количество переданных данных).
enum PluginErrorCode : int32_t
{
    PLUGIN_ERR_NONE = 0,  // Нормальное завершение
    PLUGIN_ERR_INVALID_METHOD_CALL_ID = -1,
    PLUGIN_ERR_INCORRECT_RUNTIME_ERROR = -2,
    PLUGIN_ERR_BUFFER_TOO_SMALL = -3,
    PLUGIN_ERR_UNSUPPORTED_TYPE = -4,
    PLUGIN_ERR_INVALID_SOURCE_FIELD = -5,
    PLUGIN_ERR_INVALID_TARGET_FIELD = -6,
    PLUGIN_ERR_INVALID_CONTEXT = -7,
    PLUGIN_ERR_INVALID_ARGUMENT_INDEX = -8,
    PLUGIN_ERR_IT_IS_NOT_STRING = -9,
    PLUGIN_ERR_INVALID_REQUEST = -10,
    PLUGIN_ERR_METHOD_NOT_FOUND = -11
};

// Перечисление, определяющее типы запросов к информирующей функции втыкалы GetPluginInfo().
enum PluginInfoRequest : uint32_t
{
    PLUG_REQUEST_PLUGIN_NAME = 1,           // Получение имени втыкалы.
    PLUG_REQUEST_CALL_FUNCTION_NAME = 2,    // Получение имени функции, выполняющей вызов методов данной втыкалы (поддерживается для втыкалы, размещённой в DLL).
    PLUG_REQUEST_CALL_FUNCTION_ADDR = 3,    // Получение адреса функции, выполняющей вызов методов данной втыкалы (поддерживается для втыкалы, сформированных в памяти).
    PLUG_REQUEST_METHOD_LIST = 4,           // Список имён методов класса втыкалы, доступных для вызова.
    PLUG_REQUEST_METHOD_PARAMS = 5,         // Характеристики параметров некоторого метода, предоставляемого втыкалой для обращения.
    PLUG_REQUEST_USER_VALUE = 0x80000000    // С этого индекса начинается область нестандартных, пользовательских запросов.
};

// Перечисление возможных проверок фактических параметров метода по их количеству.
enum MethodParamCheckMode : uint32_t
{ // Тип требуемых проверок.
    PARAM_CHECK_NONE = 0,                           // Проверок не выполнять.
    PARAM_CHECK_QUANTITY_EQUAL = 1,                 // Количество фактических параметров должно строго совпадать с указанным.
    PARAM_CHECK_QUANTITY_LESS_EQ = 2,               // Количество параметров может быть менее или равным указанному.
    PARAM_CHECK_QUANTITY_GREATER_EQ = 3,            // Количество параметров может быть более или равным указанному.
    PARAM_CHECK_TYPE = 4,                           // Проверять соответствие типа для всех фактических параметров.
    PARAM_CHECK_TYPE_QUANTITY_EQUAL = 5,            // Количество фактических параметров должно строго совпадать с указанным,
                                                    // а также производится проверка их всех на соответствие требуемым типам.
    PARAM_CHECK_TYPE_QUANTITY_LESS_EQ = 6,          // Аналогично сочетает требование на количество параметров (должно быть менее
                                                    // или равным указанному) с проверкой типового соответствия.
    PARAM_CHECK_TYPE_QUANTITY_GREATER_EQ = 7        // Сочетает требование на количество параметров (должно быть более или равным
                                                    // указанному) с проверкой типового соответствия.
};

// Перечисление возможных проверок фактических параметров метода по их типу (соответствию требуемому типу).
enum MethodParamType : uint32_t
{
    PARAM_TYPE_ANY = 0,                             // Контроль типа параметра не выполняется.
    PARAM_TYPE_NUMERIC = 1,                         // Параметр может быть числовым.
    PARAM_TYPE_STRING = 2,                          // Параметр может быть строковым.
    PARAM_TYPE_LOGICAL = 4,                         // Параметр может быть логическим значением.
    PARAM_TYPE_NONE = 8,                            // Параметр может быть пустым (ничего не содержать или, иначе, содержать значение None).
    // Следующие значения перечисления предназначены для тех случаев, если фактический параметр может содержать различные типы значений.
    PARAM_TYPE_NUMERIC_STRING = PARAM_TYPE_NUMERIC | PARAM_TYPE_STRING,
    PARAM_TYPE_NUMERIC_LOGICAL = PARAM_TYPE_NUMERIC | PARAM_TYPE_LOGICAL,
    PARAM_TYPE_STRING_LOGICAL = PARAM_TYPE_STRING | PARAM_TYPE_LOGICAL,
    PARAM_TYPE_NUMERIC_NONE = PARAM_TYPE_NUMERIC | PARAM_TYPE_NONE,
    PARAM_TYPE_STRING_NONE = PARAM_TYPE_STRING | PARAM_TYPE_NONE,
    PARAM_TYPE_LOGICAL_NONE = PARAM_TYPE_LOGICAL | PARAM_TYPE_NONE,
    PARAM_TYPE_NUMERIC_STRING_NONE = PARAM_TYPE_NUMERIC | PARAM_TYPE_STRING | PARAM_TYPE_NONE,
    PARAM_TYPE_NUMERIC_LOGICAL_NONE = PARAM_TYPE_NUMERIC | PARAM_TYPE_LOGICAL | PARAM_TYPE_NONE,
    PARAM_TYPE_STRING_LOGICAL_NONE = PARAM_TYPE_STRING | PARAM_TYPE_LOGICAL | PARAM_TYPE_NONE,
    PARAM_TYPE_NUMERIC_STRING_LOGICAL = PARAM_TYPE_NUMERIC | PARAM_TYPE_STRING | PARAM_TYPE_LOGICAL,
    PARAM_TYPE_NUMERIC_STRING_LOGICAL_NONE = PARAM_TYPE_NUMERIC | PARAM_TYPE_STRING | PARAM_TYPE_LOGICAL | PARAM_TYPE_NONE
};

enum ThrowMessageNumber : uint32_t
{
    THRM_UNKNOWN = 0,
    THRM_NOT_SUPPORT_FREE_FUNCTION,
    THRM_ARRAY_MUST_HAVE_DIMS,
    THRM_MAP_CTOR_HAS_NO_PARAMS,
    THRM_STR_HAS_ONE_PARAM,
    THRM_UNKNOWN_METHOD_CALL,
    THRM_POINTER_RET_TO_VAL_DENIED,
    THRM_POINTER_RET_TOL_LOCAL_VAR_DENIED,
    THRM_BASE_CLASS,
    THRM_NOT_FOUND_FOR_CLASS,
    THRM_CLASS,
    THRM_ALREADY_EXISTS,
    THRM_METHOD_NOT_FOUND,
    THRM_INDIRECT_ASSIGN_ERROR,
    THRM_VARIABLE_NOT_FOUND,
    THRM_IMPOSSIBLE_ADDITION,
    THRM_IMPOSSIBLE_SUBTRACTION,
    THRM_IMPOSSIBLE_MULTIPLICATION,
    THRM_IMPOSSIBLE_DIVISION,
    THRM_IMPOSSIBLE_COMPARE_EQUAL,
    THRM_IMPOSSIBLE_COMPARE_LESS,
    THRM_DIVISION_BY_ZERO,
    THRM_IMPOSSIBLE_MODULO_DIVISION,
    THRM_MODULO_DIVISION_BY_ZERO,
    THRM_NOT_DIGIT_SIZES,
    THRM_INVALID_ARRAY_INDEX,
    THRM_PUSH_BACK_ONE_DIM_ONLY,
    THRM_BACK_ONE_DIM_ONLY,
    THRM_POP_BACK_ONE_DIM_ONLY,
    THRM_ARRAY_IS_EMPTY,
    THRM_ITERATOR_IN_PROGRESS_INSERT,
    THRM_ITERATOR_IN_PROGRESS_ERASE,
    THRM_PARAMS_TYPE_INCONSISTENCY,
    THRM_INVALID_PARAMS_COUNT,
    THRM_INVALID_PARAM_VALUE,
    THRM_INVALID_PARAM_TYPE,
    THRM_METHOD,
    THRM_ARGUMENTS,
    THRM_DEMAND_EQUAL,
    THRM_DEMAND_LESS_OR_EQUAL,
    THRM_DEMAND_GREATER_OR_EQUAL,
    THRM_PARAMETER,
    THRM_OF_METHOD,
    THRM_HAVE_INCOMPATIBLE_TYPE,
    THRM_DEMAND_ONE_ARGUMENT,
    THRM_FIRST_PARAM_OF_METHOD,
    THRM_MUST_BE_ITERATOR,
    THRM_IN_METHOD,
    THRM_ITERATOR_INVALID,
    THRM_MATH_CTOR_HAS_NO_PARAMS,
    THRM_INCORRECT_TOKEN_LIST,
    THRM_INVALID_IMPORT_FILENAME,
    // Коды ошибок, связанные с неполадками при загрузке и обработке втыкал.
    THRM_DYNAMIC_LIBRARY_NOT_LOADED,
    THRM_LOAD_PLUGIN_LIST_NOT_FOUND,
    THRM_INVALID_PLUGIN_DATA,
    //
    THRM_INCLUDE_INVALID_PARAMS,
    THRM_SHIFT_INVALID_PARAMS,
    THRM_RAISE_CALL,
    THRM_QUALIFIER_NOT_ANCESTOR,
    THRM_AMBIGUOUS_OVERLOAD,
    THRM_METHOD_NOT_COROUTINE,
    THRM_SPECIAL_METHOD_CANT_COROUTINE,
    THRM_OBJECT_CTOR_HAS_NO_PARAMS,
    THRM_URGENT_TERMINATE,
    THRM_MAX_VALUE
};

#pragma pack(push, 1)

struct RequestMethodParams
{ // Структура входных данных запроса характеристик фактических параметров методов втыкалы.
    const char* method_name = nullptr;
    uint32_t method_ordinal = 0;
};

struct PluginMethodDefiner
{ // "Внешняя" (применяемая для обмена с функциями втыкалы) структура описания некоторого метода, предоставляемого классом-втыкалой.
    uint32_t arg_count_min = 0;       // Минимально допустимое количество его параметров.
    uint32_t arg_count_max = 0;       // Максимально допустимое количество его параметров.
    // Если метод имеет фиксированное и однозначно определённое количество параметров, можно выполнить контроль соответствия их
    // фактического типа требуемому.
    // Режим проверки допустимости фактических параметров метода (один из членов перечислимого типа MethodParamCheckMode).
    uint32_t check_mode = static_cast<uint32_t>(MethodParamCheckMode::PARAM_CHECK_NONE);
    // Список, указывающий допустимый тип для очередного фактического параметра.
    uint32_t param_types_count = 0;     // Размер списка param_types (количество его элементов).
    // ..............
    // В сформированном ответе сразу вслед за этой фиксированной структурой следует тело списка контроля типов фактических параметров
    // метода втыкалы. Содержит ровно param_types_count значений типа uint32_t, эквивалентных членам перечисления MethodParamType.
};

#pragma pack(pop)

// Функциональный тип головной функции динамической библиотеки, содержащей одну или несколько втыкал. Наличие этого уровня абстракции
// для динамических библиотек предусмотрено именно с целью возможности объединять в одну такую библиотеку сразу целый набор из
// нескольких втыкал.
using FuncGetPluginInfoNames = const char* (*)(uint32_t load_level);
// Функциональный тип, определяющий информирующую функцию втыкалы.
using PluginGetInfoFunc = int32_t(*)(uint32_t request_type, void* source_area, int32_t source_length, void* target_area, int32_t target_length);
// Функциональный тип, определяющий "вызывную" функцию, выполняющую вызов методов класса некоторой втыкалы.
using PluginCallMethodFunc = void(*)(const char* method_name, uintptr_t plugin_method_call_id);

// Ожидаемое имя головной функции динамически загружаемой библиотеки со втыкалами.
#define PLUGINS_GET_INFO_FUNCTION "GetPluginsInfoFunction"
// Ряд стандартных имён некоторых особых методов класса специального назначения.
#define  PLUGIN_INIT_METHOD "__init__"
#define  PLUGIN_DESTROY_METHOD "__destroy__"
#define  PLUGIN_ADD_METHOD "__add__"
#define  PLUGIN_EQUAL_CMP_METHOD "__eq__"
#define  PLUGIN_LESS_CMP_METHOD "__lt__"
#define  PLUGIN_STR_FUNCTION_METHOD "__str__"

// Объявления вспомогательных функций для работы со втыкалами. Они экспортируются ядром Муфлона и импортируются динамическими бибилиотеками
// самих втыкал.
extern "C"
{
    // Данная экспортируемая ядром функция возвращает условный идентификатор объекта класса втыкалы, к которому относится вызов с
    // идентом plugin_method_call_id.
    HELPERS_EXPORT_IMPORT uintptr_t PluginGetInstanceId(uintptr_t plugin_method_call_id);
    // Функция экспортируется ядром, импортируется втыкалой и вызывается изнутри её методов для передачи исполнительской среде информации
    // о том, что работа текущего метода завершилась событием, которое после его завершения должно привести к выбросу исключения msg_num
    // с сообщением except_text.
    HELPERS_EXPORT_IMPORT int32_t PluginSetRuntimeError(uintptr_t plugin_method_call_id, uint32_t msg_num, const char* except_text);

    HELPERS_EXPORT_IMPORT int32_t PluginSetResultValue(uintptr_t plugin_method_call_id, uint32_t result_type, void* source_field, int32_t source_length);

    HELPERS_EXPORT_IMPORT int32_t PluginParamsCount(uintptr_t plugin_method_call_id);

    HELPERS_EXPORT_IMPORT int32_t PluginParamType(uintptr_t plugin_method_call_id, uint32_t arg_number);

    HELPERS_EXPORT_IMPORT int32_t PluginParamStringSize(uintptr_t plugin_method_call_id, uint32_t arg_number);

    HELPERS_EXPORT_IMPORT int32_t PluginParamGetValue(uintptr_t plugin_method_call_id, uint32_t arg_number, void* target_field, int32_t target_length);
}

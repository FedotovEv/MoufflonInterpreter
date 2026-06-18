#pragma once

// В этом заголовке находятся необходимые макросы, константы, структуры данных, объявления вспомогательных функции и прочий синтаксический сахар,
// предназначенный для написания втыкал, а также облегчающий использование втыкал другими частями комплекса. Некоторые особенности структуры
// данного заголовка и использованных в нём грамматических конструкций связаны с тем, что он может использоваться и должен правильно
// транслироваться как в C, так и в C++-модулях.

#include <stdint.h>

#if defined (_WIN64) || defined(_WIN32)
    #define MYTHLON_KERNEL_EXPORT __declspec(dllexport)
    #define MYTHLON_PLUGIN_IMPORT __declspec(dllimport)
#else
    #define MYTHLON_KERNEL_EXPORT
    #define MYTHLON_PLUGIN_IMPORT
#endif

// Перечисление типов значений, которые могут быть переданы методам втыкал, а также приняты от них как результаты их работы.
#ifdef __cplusplus
    enum ObjectType : uint32_t
#else
    enum ObjectType
#endif
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
#ifdef __cplusplus
    enum PluginErrorCode : int32_t
#else
    enum PluginErrorCode
#endif
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
#ifdef __cplusplus
    enum PluginInfoRequest : uint32_t
#else
    enum PluginInfoRequest
#endif
{
    PLUG_REQUEST_PLUGIN_NAME = 1,           // Получение имени втыкалы.
    PLUG_REQUEST_CALL_FUNCTION_NAME = 2,    // Получение имени функции, выполняющей вызов методов данной втыкалы (поддерживается для втыкалы, размещённой в DLL).
    PLUG_REQUEST_CALL_FUNCTION_ADDR = 3,    // Получение адреса функции, выполняющей вызов методов данной втыкалы (поддерживается для втыкалы, сформированных в памяти).
    PLUG_REQUEST_METHOD_LIST = 4,           // Список имён методов класса втыкалы, доступных для вызова.
    PLUG_REQUEST_METHOD_PARAMS = 5,         // Характеристики параметров некоторого метода, предоставляемого втыкалой для обращения.
    PLUG_REQUEST_HELPER_FUNCTIONS = 6,      // Передача втыкале указателей на вспомогательные функции ядра для организации обмена информацией с ним.
    PLUG_REQUEST_USER_VALUE = 0x80000000    // С этого индекса начинается область нестандартных, пользовательских запросов.
};

// Перечисление возможных проверок фактических параметров метода по их количеству.
#ifdef __cplusplus
    enum MethodParamCheckMode : uint32_t
#else
    enum MethodParamCheckMode
#endif
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
    PARAM_CHECK_TYPE_QUANTITY_GREATER_EQ = 7,       // Сочетает требование на количество параметров (должно быть более или равным
                                                    // указанному) с проверкой типового соответствия.
    PARAM_CHECK_TYPE_ONLY_FOR_MIN_ARGS = 8          // Проверять тип только для первых arg_count_min фактических парметров.
};

// Перечисление возможных проверок фактических параметров метода по их типу (соответствию требуемому типу).
#ifdef __cplusplus
    enum MethodParamType : uint32_t
#else
    enum MethodParamType
#endif
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

#ifdef __cplusplus
    enum ThrowMessageNumber : uint32_t
#else
    enum ThrowMessageNumber
#endif
{
    THRM_UNKNOWN = 0,                       // Нет ошибки.
    // Общие ошибки системного происхождения.
    THRM_MEMORY_ALLOCATION_ERROR,           // Сбой при выделении динамической памяти.
    // Неверная характеристика параметра(ов) метода - неверное их полное количество, недопустимый тип, значение или длина
    // какого-либо из них.
    THRM_INVALID_PARAMS_COUNT,              // Несответствующее количество переданных параметров.
    THRM_INVALID_PARAM_VALUE,               // Некорректное величина параметра.
    THRM_INVALID_PARAM_TYPE,                // Недопустимый тип параметра.
    THRM_INVALID_PARAM_LENGTH,              // Недопустимая длина параметра (как правило, чрезмерная длина переданной строки).
    THRM_ARRAY_SIZE_NOT_NUMERIC,            // Количество элементов в массиве должно задаваться числами.
    THRM_PARAMS_TYPE_INCONSISTENCY,         // Несогласованность типов параметров метода (их расхождение с требованиям).
    THRM_ARRAY_MUST_HAVE_DIMS,              // Для массива array при конструировании нужно указать размерность.
    THRM_MAP_CTOR_HAS_NO_PARAMS,            // Конструктор ассоциативного массива (словаря) map не имеет параметров.
    THRM_MATH_CTOR_HAS_NO_PARAMS,           // Конструктор класса математической коллекции math не имеет аргументов.
    THRM_STRINGOPS_CTOR_HAS_NO_PARAMS,      // Конструктор класса строковых преобразователей StringOps также не имеет аргументов.
    THRM_OBJECT_CTOR_HAS_NO_PARAMS,         // Конструктор данного класса не должен иметь аргументов.
    THRM_STR_HAS_ONE_PARAM,                 // Функция str() должна быть вызвана строго с единственным аргументом.
    THRM_IS_SAME_TARGET_HAS_TWO_PARAMS,     // Функция IsSameTarget() должна иметь ровно два параметра.
    // Общие грамматические и синтаксические ошибки разбора исходного текста.
    THRM_SYNTAX_ERROR,                      // Общая синтаксическая ошибка (при отсутствии более детального определения).
    THRM_VARIABLE_NOT_FOUND,                // Обращение к недоступной переменной (отсутствующей в данный момент в области видимости).
    THRM_METHOD_NOT_FOUND,                  // Попытка обращения к неизвестному методу класса.
    THRM_FREE_FUNCTION_NOT_FOUND,           // Попытка вызова неизвестной свободной функции.
    THRM_QUALIFIER_NOT_ANCESTOR,            // Уточнитель метода не является предком этого класса.
    THRM_AMBIGUOUS_OVERLOAD,                // Неоднозначная перегрузка метода.
    THRM_FIELD_NOT_FOUND,                   // Обращение к несуществующему или недоступному полю.
    THRM_POINTER_RET_TO_VAL_DENIED,         // Ссылка на временное значение недопустима.
    THRM_POINTER_RET_TOL_LOCAL_VAR_DENIED,  // Ссылка на локальную переменную метода невозможна.
    THRM_INDIRECT_ASSIGN_ERROR,             // Ошибка косвенного присваивания.
    // Недопустимые и невыполнимые операции.
    THRM_IMPOSSIBLE_ADDITION,                   // Невозможно произвести такое сложение.
    THRM_IMPOSSIBLE_SUBTRACTION,                // Невозможно произвести такое вычитание.
    THRM_IMPOSSIBLE_MULTIPLICATION,             // Невозможно произвести указанное умножение.
    THRM_IMPOSSIBLE_DIVISION,                   // Невозможно произвести указанное деление.
    THRM_IMPOSSIBLE_COMPARE_EQUAL,              // Невозможно выполнить сравнение на равенство с такими операндами.
    THRM_IMPOSSIBLE_COMPARE_LESS,               // Невозможно выполнить сравнение на "меньше" с такими операндами.
    THRM_DIVISION_BY_ZERO,                      // Деление на нуль.
    THRM_IMPOSSIBLE_MODULO_DIVISION,            // Невозможно выполнить такое деление по модулю.
    THRM_MODULO_DIVISION_BY_ZERO,               // Целочисленное деление на нуль.
    THRM_SHIFT_INVALID_PARAMS,                  // Недопустимые параметры для операции сдвига.
    THRM_OVERFLOW,                              // Математическое переполнение при вычислениях.
    THRM_NUMBER_STRING_CONVERSION_ERROR,        // Ошибка при преобразовании числа в строку или обратно (из строкового представления в числовое).
    THRM_CONTEXT_OUT_FAIL,                      // Печать данных в контекст по какой-то причине завершилась неудачно.
    // Ошибки при работе с массивами и словарями.
    THRM_INVALID_ARRAY_INDEX,                   // Индекс массива некорректен или лежит вне пределов его размера.
    THRM_PUSH_BACK_ONE_DIM_ONLY,                // PushBack() применим только для одномерных массивов.
    THRM_BACK_ONE_DIM_ONLY,                     // Back() применим только для одномерных массивов.
    THRM_POP_BACK_ONE_DIM_ONLY,                 // PopBack() допустим только для одномерных массивов.
    THRM_ARRAY_IS_EMPTY,                        // Массив пуст.
    THRM_CURSOR_IN_PROGRESS_INSERT,             // Метод insert выполнить невозможно, так как имеются активные курсоры.
    THRM_CURSOR_IN_PROGRESS_ERASE,              // Метод erase выполнить невозможно, так как имеются активные курсоры.
    // Ошибки разбора аргументов директив синтаксического анализатора.
    THRM_INCORRECT_TOKEN_LIST,                  // Недопустимый список лексем-параметров директивы.
    THRM_INCLUDE_INVALID_PARAMS,                // Невалидные параметры директивы "include".
    // Коды ошибок, связанные с неполадками при загрузке и обработке втыкал.
    THRM_INVALID_IMPORT_FILENAME,               // Некорректное имя файла в директиве "import".
    THRM_DYNAMIC_LIBRARY_NOT_LOADED,            // Динамическая разделяемая библиотека не загружена (при загрузке возникла системная ошибка).
    THRM_LOAD_PLUGINS_LIST_NOT_FOUND,           // Неверный формат динамической библиотеки коллекции втыкал - не найдена корневая функция-информатор GetPluginsInfoFunction.
    // Следующая группа кодов свидетельствует о ситуации получения невалидного ответа на какой-либо запрос к функции-информатору отдельной втыкалы.
    THRM_INVALID_PLUGIN_INFO_FUNC,              // Некорректное имя информирующей функции отдельной втыкалы - слишком длинное, содержит недопустимые символы
                                                // или отсутствует среди экспортируемых динамической библиотекой.
    THRM_INVALID_PLUGIN_METHOD_LIST,            // Недопустимый формат списка методов втыкалы или он содержит некорректные имена функций.
    THRM_INCORRECT_METHOD_DEFINER,              // Получен невалидный описатель парметров конкретного метода - слишком короткий либо с явно недопустимыми значениями полей.
    THRM_INVALID_PLUGIN_NAME,                   // У информатора получено некорректное имя втыкалы - пустое, слишком длинное или с недопустимыми символами.
    THRM_INVALID_PLUGIN_CALL_FUNC,              // Некорректное имя вызывающей функции отдельной втыкалы - слишком длинное, содержит недопустимые символы
                                                // или отсутствует среди экспортируемых динамической библиотекой.
    // Специфические проблемы при работе с сопрограммами и ждунами.
    THRM_METHOD_NOT_COROUTINE,                  // Метод не является сопрограммой.
    THRM_SPECIAL_METHOD_CANT_COROUTINE,         // Методы специального назначения не могут быть сопрограммами.
    THRM_OBJECT_NOT_AWAITABLE,                  // Данный объект не относится к стопусловным (не является ждуном).
    // Условные коды для нормальных рабочих процедур, обрабатываемых как ошибки.
    THRM_RAISE_CALL,                            // Исполнение оператора выброса исключения raise.
    THRM_URGENT_TERMINATE,                      // Экстренное завершение работы программы.
    // Завершение блока кодов возможных ошибок разбора и исполнения программы, которые при своём возникновении порождают такие коды.
    THRM_MAX_ERROR = THRM_URGENT_TERMINATE,     // Максимальное значение единичного (определяющего тип отдельной ошибки) кода ошибки.
    // Предложения для формирования составных сообщений об ошибках.
    THRM_BASE_CLASS,                // "Базовый класс "
    THRM_NOT_FOUND_FOR_CLASS,       // " не найден для класса "
    THRM_CLASS,                     // "Класс "
    THRM_FUNCTION,                  // "Функция "
    THRM_ALREADY_EXISTS,            // "уже сущестует"
    THRM_METHOD,                    // "Метод "
    THRM_ARGUMENTS,                 // " аргументов"
    THRM_DEMAND_EQUAL,              // " требует "
    THRM_DEMAND_LESS_OR_EQUAL,      // " требует не более "
    THRM_DEMAND_GREATER_OR_EQUAL,   // " требует не менее "
    THRM_PARAMETER,                 // "Параметр "
    THRM_OF_METHOD,                 // " метода "
    THRM_HAVE_INCOMPATIBLE_TYPE,    // " имеет несоответствующий тип"
    THRM_DEMAND_ONE_ARGUMENT,       // " требует 1 аргумент"
    THRM_FIRST_PARAM_OF_METHOD,     // "Параметр 1 метода "
    THRM_MUST_BE_CURSOR,            // " должен быть курсором"
    THRM_IN_METHOD,                 // "В методе "
    THRM_CURSOR_INVALID,            // " курсор недействителен"
    // Конец определения списка кодов ошибок.
    THRM_MAX_VALUE = THRM_CURSOR_INVALID      // Абсолютно максимальное значение данного перечисления.
};

// Функциональный тип головной функции динамической библиотеки, содержащей одну или несколько втыкал. Наличие этого уровня абстракции
// для динамических библиотек предусмотрено именно с целью возможности объединять в одну такую библиотеку сразу целый набор из
// нескольких втыкал.
typedef const char* (*FuncGetPluginInfoNames)(uint32_t load_level);
// Функциональный тип, определяющий информирующую функцию втыкалы.
typedef int32_t(*PluginGetInfoFunc)(uint32_t request_type, void* source_area, int32_t source_length, void* target_area, int32_t target_length);
// Функциональный тип, определяющий "вызывную" функцию, выполняющую вызов методов класса некоторой втыкалы.
typedef void(*PluginCallMethodFunc)(const char* method_name, uintptr_t plugin_method_call_id);

// Ожидаемое имя головной функции динамически загружаемой библиотеки со втыкалами.
#define PLUGINS_GET_INFO_FUNCTION "GetPluginsInfoFunction"
// Ряд стандартных имён некоторых особых методов класса специального назначения.
#define  PLUGIN_INIT_METHOD "__init__"
#define  PLUGIN_DESTROY_METHOD "__destroy__"
#define  PLUGIN_ADD_METHOD "__add__"
#define  PLUGIN_EQUAL_CMP_METHOD "__eq__"
#define  PLUGIN_LESS_CMP_METHOD "__lt__"
#define  PLUGIN_STR_FUNCTION_METHOD "__str__"

// Функциональные типы (тип указателей на функции), соответствующие вспомогательным функциям, экспортируемым ядром Муфлона для нужд подключаемых
// к нему втыкал.
typedef uintptr_t(*PluginGetInstanceIdFunc)(uintptr_t plugin_method_call_id);
typedef int32_t(*PluginSetRuntimeErrorFunc)(uintptr_t plugin_method_call_id, uint32_t msg_num, const char* except_text);
typedef int32_t(*PluginSetResultValueFunc)(uintptr_t plugin_method_call_id, uint32_t result_type, void* source_field, int32_t source_length);
typedef int32_t(*PluginParamsCountFunc)(uintptr_t plugin_method_call_id);
typedef int32_t(*PluginParamTypeFunc)(uintptr_t plugin_method_call_id, uint32_t arg_number);
typedef int32_t(*PluginParamGetValueFunc)(uintptr_t plugin_method_call_id, uint32_t arg_number, void* target_field, int32_t target_length);
typedef int32_t(*PluginParamStringSizeFunc)(uintptr_t plugin_method_call_id, uint32_t arg_number);
typedef int32_t(*PluginPrintToContextFunc)(uintptr_t plugin_method_call_id, uint32_t source_type, void* source_field, int32_t source_length);

#pragma pack(push, 1)

#ifdef __cplusplus
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

    struct PluginHelperFunctions
    {
        PluginGetInstanceIdFunc get_instance_func = nullptr;
        PluginSetRuntimeErrorFunc set_runtime_error_func = nullptr;
        PluginSetResultValueFunc set_result_value_func = nullptr;
        PluginParamsCountFunc params_count_func = nullptr;
        PluginParamTypeFunc param_type_func = nullptr;
        PluginParamGetValueFunc param_get_value_func = nullptr;
        PluginParamStringSizeFunc param_string_size_func = nullptr;
        PluginPrintToContextFunc print_to_context_func = nullptr;
    };
#else
    struct RequestMethodParams
    { // Структура входных данных запроса характеристик фактических параметров методов втыкалы.
        const char* method_name;
        uint32_t method_ordinal;
    };

    struct PluginMethodDefiner
    { // "Внешняя" (применяемая для обмена с функциями втыкалы) структура описания некоторого метода, предоставляемого классом-втыкалой.
        uint32_t arg_count_min;       // Минимально допустимое количество его параметров.
        uint32_t arg_count_max;       // Максимально допустимое количество его параметров.
        // Если метод имеет фиксированное и однозначно определённое количество параметров, можно выполнить контроль соответствия их
        // фактического типа требуемому.
        // Режим проверки допустимости фактических параметров метода (один из членов перечислимого типа MethodParamCheckMode).
        uint32_t check_mode;
        // Список, указывающий допустимый тип для очередного фактического параметра.
        uint32_t param_types_count;     // Размер списка param_types (количество его элементов).
        // ..............
        // В сформированном ответе сразу вслед за этой фиксированной структурой следует тело списка контроля типов фактических параметров
        // метода втыкалы. Содержит ровно param_types_count значений типа uint32_t, эквивалентных членам перечисления MethodParamType.
    };

    struct PluginHelperFunctions
    {
        PluginGetInstanceIdFunc get_instance_func;
        PluginSetRuntimeErrorFunc set_runtime_error_func;
        PluginSetResultValueFunc set_result_value_func;
        PluginParamsCountFunc params_count_func;
        PluginParamTypeFunc param_type_func;
        PluginParamGetValueFunc param_get_value_func;
        PluginParamStringSizeFunc param_string_size_func;
        PluginPrintToContextFunc print_to_context_func;
    };
#endif

#pragma pack(pop)

// Объявления вспомогательных функций для работы со втыкалами. Они экспортируются ядром Муфлона и импортируются динамическими бибилиотеками
// самих втыкал.
#ifndef MYTHLON_PLUGIN
    // Данные функцию объявляются и определяются только в ядре МУФЛОНА (не во втыкалах) и экспортируются им для нужд втыкал, если по какой-то причине
    // они предпочтут использовать их явный импорт через системные механизмы ОС.
    extern "C"
    {
        // Данная экспортируемая ядром функция возвращает условный идентификатор объекта класса втыкалы, к которому относится вызов с
        // идентом plugin_method_call_id.
        MYTHLON_KERNEL_EXPORT uintptr_t PluginGetInstanceId(uintptr_t plugin_method_call_id);
        // Функция экспортируется ядром, импортируется втыкалой и вызывается изнутри её методов для передачи исполнительской среде информации
        // о том, что работа текущего метода завершилась событием, которое после его завершения должно привести к выбросу исключения msg_num
        // с сообщением except_text.
        MYTHLON_KERNEL_EXPORT int32_t PluginSetRuntimeError(uintptr_t plugin_method_call_id, uint32_t msg_num, const char* except_text);
        // Экспортируемая ядром функция, служащая для установки возвращаемого методом значения при его нормальном завершении.
        MYTHLON_KERNEL_EXPORT int32_t PluginSetResultValue(uintptr_t plugin_method_call_id, uint32_t result_type, void* source_field, int32_t source_length);
        // Функция, возвращающая количество входных фактических параметров, переданных методу при его вызове.
        MYTHLON_KERNEL_EXPORT int32_t PluginParamsCount(uintptr_t plugin_method_call_id);
        // Функция, служащая цели получения информации о типе фактического параметра с индексом (номером, базированным к нулю) arg_number, переданном методу
        // при его вызове.
        MYTHLON_KERNEL_EXPORT int32_t PluginParamType(uintptr_t plugin_method_call_id, uint32_t arg_number);
        // Функция, посредством которой производится получение значения фактического параметра с индексом arg_number. При работе функции значение этого параметра
        // копируется в целевое поле (target_field, target_length).
        MYTHLON_KERNEL_EXPORT int32_t PluginParamGetValue(uintptr_t plugin_method_call_id, uint32_t arg_number, void* target_field, int32_t target_length);
        // Дополнительная функция для того случая, если фактический параметр arg_number метода является строкой. В таком случае функция возвращает её длину.
        MYTHLON_KERNEL_EXPORT int32_t PluginParamStringSize(uintptr_t plugin_method_call_id, uint32_t arg_number);
        // Функция печати (направления в выходной поток контекста, использованного при вызове метода) переменной типа source_type, расположенной во входном
        // буфере (source_field, source_length).
        MYTHLON_KERNEL_EXPORT int32_t PluginPrintToContext(uintptr_t plugin_method_call_id, uint32_t source_type, void* source_field, int32_t source_length);
    }
#endif

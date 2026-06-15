
// Двоичное дополнение - "втыкало" - для интерпретатора МУФЛОН. Написан на языке C (именно C, а не C++) для демонстрации
// возможности написания таких втыкал на иных языках, отличных от C++, на котором реализован сам МУФЛОН.
// После подключения обеспечивает простую работу с файловой системой средствами библиотеки ввода-вывода в стиле C.

#include "file_plugin.h"
#include "string.h"

#ifndef true
    // Начиная с C23 этот заголовок не нужен.
    #include "stdbool.h"
#endif

#define ArraySize(x) (sizeof(x) / sizeof(x[0]))

#ifdef _MSC_VER
    #define ulltoa _ui64toa
#else
    char* ui64toa_func(long long value, char* str, int base)
    {

        char format[16] = { "%ll`000b" };    //'B' - знаковый, 'b' - беззнаковый.
        char* p = { "%llu" };

        if (base != 10)
        {
            format[6] = '0' + base % 10;
            format[5] = '0' + (base /= 10) % 10;
            format[4] = '0' + (base /= 10) % 10;

            p = &format[0];
        }
        sprintf(str, p, value);
        return str;
    }

    #define ulltoa ui64toa_func
#endif

// Блок текстовых C-строк с информацией об ошибках.
static const char memory_allocation[] = "Ошибка при выделении памяти";
static const char method_not_found[] = "Метод не найден";
static const char type_not_supported[] = "Тип не поддерживается";
static const char method_has_no_params[] = "Метод не имеет параметров";
static const char method_has_one_param[] = "Метод принимает строго один параметр";
static const char method_has_two_param[] = "Метод принимает строго два параметра";
static const char method_has_one_or_two_param[] = "Метод принимает один или два параметра";
//
static const char filename_not_string[] = "Имя файла должно быть строкой";
static const char filemode_not_string[] = "Режим доступа к файлу должен быть строкой";
//
static const char bytes_count_not_number[] = "Количество байтов должно быть числом";
//
static const char target_point_not_number[] = "Целевая точка перемещения должна быть числом";
static const char seekmode_not_number[] = "Режим позиционирования должен быть числом";
static const char invalid_seekmode[] = "Недопустимый режим позиционирования";
//
static const char file_not_opened[] = "Файл не открыт";
static const char string_too_long[] = "Строка слишком длинная";

// Набор C-строк со стандартными именами специальных методов классов МУФЛОНА.
static const char plugin_init_method_name[] = PLUGIN_INIT_METHOD;
static const char plugin_destroy_method_name[] = PLUGIN_DESTROY_METHOD;
static const char plugin_str_function_name[] = PLUGIN_STR_FUNCTION_METHOD;
static const char plugin_add_method_name[] = PLUGIN_ADD_METHOD;

struct FilePluginStatus* plugin_status_list = NULL; // Указатель на голову списка экземпляров объектов данной втыкалы.
// Указатели на сервисные функции среды, в которую включена наша библиотека.
struct PluginHelperFunctions helper_funcs = 
{
    .get_instance_func = NULL,
    .set_runtime_error_func = NULL,
    .set_result_value_func = NULL,
    .params_count_func = NULL,
    .param_type_func = NULL,
    .param_get_value_func = NULL,
    .param_string_size_func = NULL,
    .print_to_context_func = NULL
};

// Структура, описывающая отдельный метод класса втыкалы, предоставляемый им для вызова извне, со стороны
// МУФЛОН-программы (методы общего типа) или со стороны исполнительской среды (специальные методы).
#define MAX_PARAM_TYPES 8
struct PluginMethodTable
{
    const char* method_name;                                // Имя метода.
    MethodFunc method_func;                                 // Указатель на функцию-обработчик вызова этого метода.
    struct PluginMethodDefiner params_definer;              // Характеристика допустимых формальных и фактических параметров.
    enum MethodParamType method_params[MAX_PARAM_TYPES];    // Список описаний допустимых типов фактических параметров данного метода.
};

static const char* PLUGIN_CLASS_METHODS_LIST[] =
{   // Список существующих публичных методов класса втыкалы. Специальные методы (инициализатор, компаратор, конвертер в строку, и.т.д.)
    // сюда тоже можно включить, что мы и сделаем.
    plugin_init_method_name,      // Стандартное имя инициализирующего метода класса втыкалы (конструктора).
    plugin_str_function_name,     // Стандартное имя строкофикатора - преобразователя текущего состояния объекта втыкалы в строку.
    // Прочие свободно определяемые методы класса втыкалы.
    "open",
    "Open",
    "close",
    "Close",
    "read",
    "Read",
    "write",
    "Write",
    "seek",
    "Seek",
    "tell",
    "Tell",
    "rewind",
    "Rewind",
    "is_open",
    "IsOpen",
    "remove",
    "Remove",
    "rename",
    "Rename",
    "status",
    "Status",
    "eof",
    "Eof",
    "error",
    "Error",
    ""          // Обязательный пустой завершающий элемент списка.
};

// Таблица характеристик параметров методов класса данной втыкалы.
const struct PluginMethodTable plugin_method_table[] =
{
    // Специальные стандартные методы Муфлон-классов.
    {.method_name = plugin_init_method_name, .method_func = &MethodInit,
     .params_definer = {.arg_count_min = 0, .arg_count_max = 2, .check_mode = (uint32_t)PARAM_CHECK_TYPE_QUANTITY_EQUAL, .param_types_count = 2},
     .method_params = {PARAM_TYPE_STRING, PARAM_TYPE_STRING}},  // Ноль, один или два строковых аргумента.
    {.method_name = plugin_str_function_name, .method_func = &MethodStringize,
     .params_definer = {.arg_count_min = 0, .arg_count_max = 0, .check_mode = (uint32_t)PARAM_CHECK_QUANTITY_EQUAL, .param_types_count = 0}},
    // Прочие свободно определяемые методы класса втыкалы.
    {.method_name = "open", .method_func = &MethodFileOpen,     // Один или два строковых аргумента.
     .params_definer = {.arg_count_min = 1, .arg_count_max = 2, .check_mode = (uint32_t)PARAM_CHECK_TYPE_QUANTITY_EQUAL, .param_types_count = 2},
     .method_params = {PARAM_TYPE_STRING, PARAM_TYPE_STRING}},
    {.method_name = "Open", .method_func = &MethodFileOpen,     // Один или два строковых аргумента.
     .params_definer = {.arg_count_min = 1, .arg_count_max = 2, .check_mode = (uint32_t)PARAM_CHECK_TYPE_QUANTITY_EQUAL, .param_types_count = 2},
     .method_params = {PARAM_TYPE_STRING, PARAM_TYPE_STRING}},
    {.method_name = "close", .method_func = &MethodFileClose,   // Аналогично.
     .params_definer = {.arg_count_min = 0, .arg_count_max = 0, .check_mode = (uint32_t)PARAM_CHECK_QUANTITY_EQUAL, .param_types_count = 0}},
    {.method_name = "Close", .method_func = &MethodFileClose,
     .params_definer = {.arg_count_min = 0, .arg_count_max = 0, .check_mode = (uint32_t)PARAM_CHECK_QUANTITY_EQUAL, .param_types_count = 0}},
    {.method_name = "read", .method_func = &MethodFileRead,     // Один численный аргумент - длина считываемого блока.
     .params_definer = {.arg_count_min = 1, .arg_count_max = 1, .check_mode = (uint32_t)PARAM_CHECK_TYPE_QUANTITY_EQUAL, .param_types_count = 1},
     .method_params = {PARAM_TYPE_NUMERIC}},
    {.method_name = "Read", .method_func = &MethodFileRead,     // Тот же вызов read, но с большой буквы.
     .params_definer = {.arg_count_min = 1, .arg_count_max = 1, .check_mode = (uint32_t)PARAM_CHECK_TYPE_QUANTITY_EQUAL, .param_types_count = 1},
     .method_params = {PARAM_TYPE_NUMERIC}},
    {.method_name = "write", .method_func = &MethodFileWrite,    // Один аргумент относительно произвольного типа.
     .params_definer = {.arg_count_min = 1, .arg_count_max = 1, .check_mode = (uint32_t)PARAM_CHECK_QUANTITY_EQUAL, .param_types_count = 0}},
    {.method_name = "Write", .method_func = &MethodFileWrite,    // То же самое.
     .params_definer = {.arg_count_min = 1, .arg_count_max = 1, .check_mode = (uint32_t)PARAM_CHECK_QUANTITY_EQUAL, .param_types_count = 0}},
    {.method_name = "seek", .method_func = &MethodFileSeek,     // Один либо два числовых аргумента - новая позиция и способ её отсчета.
     .params_definer = {.arg_count_min = 1, .arg_count_max = 2, .check_mode = (uint32_t)PARAM_CHECK_TYPE_QUANTITY_EQUAL, .param_types_count = 2},
     .method_params = {PARAM_TYPE_NUMERIC, PARAM_TYPE_NUMERIC}},
    {.method_name = "Seek", .method_func = &MethodFileSeek,     // Аналог с большой буквы.
     .params_definer = {.arg_count_min = 1, .arg_count_max = 2, .check_mode = (uint32_t)PARAM_CHECK_TYPE_QUANTITY_EQUAL, .param_types_count = 2},
     .method_params = {PARAM_TYPE_NUMERIC, PARAM_TYPE_NUMERIC}},
    {.method_name = "tell", .method_func = &MethodFileTell,     // Аргументов не имеет.
     .params_definer = {.arg_count_min = 0, .arg_count_max = 0, .check_mode = (uint32_t)PARAM_CHECK_QUANTITY_EQUAL, .param_types_count = 0}},
    {.method_name = "Tell", .method_func = &MethodFileTell,     // Аналог.
     .params_definer = {.arg_count_min = 0, .arg_count_max = 0, .check_mode = (uint32_t)PARAM_CHECK_QUANTITY_EQUAL, .param_types_count = 0}},
    {.method_name = "rewind", .method_func = &MethodFileRewind,     // Также аргументов не имеет.
     .params_definer = {.arg_count_min = 0, .arg_count_max = 0, .check_mode = (uint32_t)PARAM_CHECK_QUANTITY_EQUAL, .param_types_count = 0}},
    {.method_name = "Rewind", .method_func = &MethodFileRewind,     // Эквивалент.
     .params_definer = {.arg_count_min = 0, .arg_count_max = 0, .check_mode = (uint32_t)PARAM_CHECK_QUANTITY_EQUAL, .param_types_count = 0}},
    {.method_name = "is_open", .method_func = &MethodFileIsOpen,     // Аргументов не принимает.
     .params_definer = {.arg_count_min = 0, .arg_count_max = 0, .check_mode = (uint32_t)PARAM_CHECK_QUANTITY_EQUAL, .param_types_count = 0}},
    {.method_name = "IsOpen", .method_func = &MethodFileIsOpen,     // Эквивалент.
     .params_definer = {.arg_count_min = 0, .arg_count_max = 0, .check_mode = (uint32_t)PARAM_CHECK_QUANTITY_EQUAL, .param_types_count = 0}},
    {.method_name = "remove", .method_func = &MethodFileRemove,     // Строго один строковый аргумент.
     .params_definer = {.arg_count_min = 1, .arg_count_max = 1, .check_mode = (uint32_t)PARAM_CHECK_TYPE_QUANTITY_EQUAL, .param_types_count = 1},
     .method_params = {PARAM_TYPE_STRING}},
    {.method_name = "Remove", .method_func = &MethodFileRemove,     // Так же.
     .params_definer = {.arg_count_min = 1, .arg_count_max = 1, .check_mode = (uint32_t)PARAM_CHECK_TYPE_QUANTITY_EQUAL, .param_types_count = 1},
     .method_params = {PARAM_TYPE_STRING}},
    {.method_name = "rename", .method_func = &MethodFileRename,     // Строго два строковых аргумента.
     .params_definer = {.arg_count_min = 2, .arg_count_max = 2, .check_mode = (uint32_t)PARAM_CHECK_TYPE_QUANTITY_EQUAL, .param_types_count = 2},
     .method_params = {PARAM_TYPE_STRING, PARAM_TYPE_STRING}},
    {.method_name = "Rename", .method_func = &MethodFileRename,     // Аналогично.
     .params_definer = {.arg_count_min = 2, .arg_count_max = 2, .check_mode = (uint32_t)PARAM_CHECK_TYPE_QUANTITY_EQUAL, .param_types_count = 2},
     .method_params = {PARAM_TYPE_STRING, PARAM_TYPE_STRING}},
    {.method_name = "status", .method_func = &MethodFileStatus,     // Аргументов не принимает.
     .params_definer = {.arg_count_min = 0, .arg_count_max = 0, .check_mode = (uint32_t)PARAM_CHECK_QUANTITY_EQUAL, .param_types_count = 0}},
    {.method_name = "Status", .method_func = &MethodFileStatus,     // Эквивалент.
     .params_definer = {.arg_count_min = 0, .arg_count_max = 0, .check_mode = (uint32_t)PARAM_CHECK_QUANTITY_EQUAL, .param_types_count = 0}},
    {.method_name = "eof", .method_func = &MethodFileEof,     // Аргументов не принимает.
     .params_definer = {.arg_count_min = 0, .arg_count_max = 0, .check_mode = (uint32_t)PARAM_CHECK_QUANTITY_EQUAL, .param_types_count = 0}},
    {.method_name = "Eof", .method_func = &MethodFileEof,     // Эквивалент.
     .params_definer = {.arg_count_min = 0, .arg_count_max = 0, .check_mode = (uint32_t)PARAM_CHECK_QUANTITY_EQUAL, .param_types_count = 0}},
    {.method_name = "error", .method_func = &MethodFileError,     // Аргументов не принимает.
     .params_definer = {.arg_count_min = 0, .arg_count_max = 0, .check_mode = (uint32_t)PARAM_CHECK_QUANTITY_EQUAL, .param_types_count = 0}},
    {.method_name = "Error", .method_func = &MethodFileError,     // Эквивалент.
     .params_definer = {.arg_count_min = 0, .arg_count_max = 0, .check_mode = (uint32_t)PARAM_CHECK_QUANTITY_EQUAL, .param_types_count = 0}}
};

// Функция проверки того, принадлежит ли аргумент arg_number вызова plugin_method_call_id к численному типу.
static bool IsArgumNumeric(uintptr_t plugin_method_call_id, uint32_t arg_number, const char* error_message)
{
    enum ObjectType param_type = (enum ObjectType)helper_funcs.param_type_func(plugin_method_call_id, arg_number);
    if (param_type != OBJECT_TYPE_INTEGER && param_type != OBJECT_TYPE_DOUBLE)
    { // Параметр arg_number не принадлежит какому-либо численному типу.
        helper_funcs.set_runtime_error_func(plugin_method_call_id, (uint32_t)THRM_INVALID_PARAM_TYPE, error_message);
        return false;
    }
    return true;
}

// Функция извлечения целочисленного аргумента с номером arg_number для вызова с идентом plugin_method_call_id.
static int GetUniIntNumber(uintptr_t plugin_method_call_id, uint32_t arg_number)
{
    int int_result = 0;
    double double_result = 0.0;

    enum ObjectType arg_type = (enum ObjectType)helper_funcs.param_type_func(plugin_method_call_id, arg_number);
    switch (arg_type)
    {
    case OBJECT_TYPE_INTEGER:
        if (helper_funcs.param_get_value_func(plugin_method_call_id, arg_number, &int_result, (int32_t)sizeof(int)) != sizeof(int))
            int_result = 0;   // Считать значение почему-то не удалось.
        break;
    case OBJECT_TYPE_DOUBLE:
        if (helper_funcs.param_get_value_func(plugin_method_call_id, arg_number, &double_result, (int32_t)sizeof(double))
            == sizeof(double))
            int_result = (int)(double_result);
        break;
    default:
        int_result = 0;
        break;
    }

    return int_result;
}

static double GetUniDoubleNumber(uintptr_t plugin_method_call_id, uint32_t arg_number)
{
    int int_result = 0;
    double double_result = 0.0;

    enum ObjectType arg_type = (enum ObjectType)helper_funcs.param_type_func(plugin_method_call_id, arg_number);
    switch (arg_type)
    {
    case OBJECT_TYPE_DOUBLE:
        if (helper_funcs.param_get_value_func(plugin_method_call_id, arg_number, &double_result, (int32_t)sizeof(double))
            != sizeof(double))
            double_result = 0.0;   // Считать значение почему-то не удалось.
        break;
    case OBJECT_TYPE_INTEGER:
        if (helper_funcs.param_get_value_func(plugin_method_call_id, arg_number, &int_result, (int32_t)sizeof(int)) == sizeof(int))
            double_result = (double)(int_result);
        break;
    default:
        double_result = 0.0;
        break;
    }

    return double_result;
}

// Небольшой наборчик процедур для работы со связным списком существующих объектов втыкалы.
// Функция поиска блока данных объекта втыкалы, соответствующего внешней оболочке с идентом external_object_id.
static struct FilePluginStatus* FindPluginStatus(uintptr_t find_external_object_id)
{
    struct FilePluginStatus* scan_plugin_status;
    for (scan_plugin_status = plugin_status_list;
         scan_plugin_status && scan_plugin_status->external_object_id != find_external_object_id;
         scan_plugin_status = scan_plugin_status->next_plugin_rec);

    return scan_plugin_status;
}

static void DeletePluginStatus(uintptr_t plugin_method_call_id, struct FilePluginStatus* plugin_instance);
// Функция вставки нового элемента (пакет полей экземпляра класса втыкалы) в список-хранилище таких объектов.
static void InsertNewPluginStatus(uintptr_t plugin_method_call_id, struct FilePluginStatus* new_plugin_instance)
{
    // Если экземпляр для данного external_object_id уже существует, то старый экземпляр предварительно уничтожается.
    // Такого при правильном функционировании программы быть не может, но, на всякий пожарный, предусмотрим и этот вариант.
    struct FilePluginStatus* old_instance = FindPluginStatus(new_plugin_instance->external_object_id);
    if (old_instance)
        DeletePluginStatus(plugin_method_call_id, old_instance);

    // Вставку производим в начало списка.
    new_plugin_instance->next_plugin_rec = plugin_status_list;
    new_plugin_instance->prev_plugin_rec = NULL;
    plugin_status_list = new_plugin_instance;
}

// Функция удаления существующего экземпяра plugin_instance из списка.
static void DeletePluginStatus(uintptr_t plugin_method_call_id, struct FilePluginStatus* plugin_instance)
{
    MethodDestroy(plugin_method_call_id, plugin_instance);

    // Вырезаем элемент из списка, связывая его предыдущий элемент (или начальный указатель, если удаляемый объект
    // первый в списке) напрямую с последующим.
    if (plugin_instance->prev_plugin_rec)
        plugin_instance->prev_plugin_rec->next_plugin_rec = plugin_instance->next_plugin_rec;
    if (plugin_status_list == plugin_instance)
        plugin_status_list = plugin_instance->next_plugin_rec;

    free(plugin_instance);
}

// Функция, полностью очищающая список активных втыкал plugin_status_list. Она нестатическая, так как будет вызываться, в том числе,
// из процедуры обслуживания точки входа динамической библиотеки.
void ClearPluginStatuses()
{
    struct FilePluginStatus* scan_plugin_instance = plugin_status_list;
    while (scan_plugin_instance)
    {
        MethodDestroy(0, scan_plugin_instance);

        struct FilePluginStatus* old_scan_plugin_instance = scan_plugin_instance;
        scan_plugin_instance = scan_plugin_instance->next_plugin_rec;

        free(old_scan_plugin_instance);
    }
    plugin_status_list = NULL;
}

// Функция производит поиск дескриптора метода find_method в массиве plugin_method_table.
static const struct PluginMethodTable* FindMethodDesc(const char* find_method_name)
{
    for (size_t method_index = 0; method_index < ArraySize(plugin_method_table); ++method_index)
    {
        if (strcmp(find_method_name, plugin_method_table[method_index].method_name) == 0)
            return &plugin_method_table[method_index];
    }
    // Метод с затребованным именем не существует.
    return NULL;
}

MYTHLON_MODULE_EXPORT const char* GetPluginsInfoFunction(uint32_t load_level)
{
    static const char* PLUGIN_INFO_FUNC_LIST[] =
    {
        "GetPluginInfo",    // Имя информирующей функции для единственной втыкалы этого модуля.
        ""                  // Обязательный пустой закрывающий элемент списка.
    };

    return PLUGIN_INFO_FUNC_LIST[0];
}

// Информирующая функция (информатор) для данной втыкалы.
MYTHLON_MODULE_EXPORT int32_t GetPluginInfo(uint32_t request_type, void* source_area, int32_t source_length, void* target_area, int32_t target_length)
{
    static const char PLUGIN_NAME[] = "FilePlugin";
    static const char EXECUTE_FUNCTION_NAME[] = "CallPluginMethod";

    switch (request_type)
    {
    case PLUG_REQUEST_PLUGIN_NAME:           // Получение имени втыкалы.
        if (target_length < (int32_t)ArraySize(PLUGIN_NAME))
            return (int32_t)PLUGIN_ERR_BUFFER_TOO_SMALL;

        strcpy((char*)target_area, PLUGIN_NAME);
        return (int32_t)ArraySize(PLUGIN_NAME);
    case PLUG_REQUEST_CALL_FUNCTION_NAME:    // Получение имени функции, выполняющей вызов методов данной втыкалы.
        if (target_length < (int32_t)(ArraySize(EXECUTE_FUNCTION_NAME)))
            return (int32_t)PLUGIN_ERR_BUFFER_TOO_SMALL;

        strcpy((char*)target_area, EXECUTE_FUNCTION_NAME);
        return (int32_t)ArraySize(EXECUTE_FUNCTION_NAME);
    case PLUG_REQUEST_METHOD_LIST:           // Список имён методов класса втыкалы, доступных для вызова.
    {
        // Рассчитаем полную длину списка, который нам нужно возвратить в ответ на данный запрос.
        size_t total_data_size = 0;
        for (size_t i = 0; i < ArraySize(PLUGIN_CLASS_METHODS_LIST); ++i)
            total_data_size += (strlen(PLUGIN_CLASS_METHODS_LIST[i]) + 1);
        if (target_length < total_data_size)
            return (int32_t)PLUGIN_ERR_BUFFER_TOO_SMALL;

        // Длина приёмного буфера достаточна.
        for (size_t method_index = 0; method_index < ArraySize(PLUGIN_CLASS_METHODS_LIST); ++method_index)
        { // Переносим по одной строки с именами методов в приёмный буфер.
            const char* method_name = PLUGIN_CLASS_METHODS_LIST[method_index];
            size_t method_name_length = strlen(method_name) + 1;
            memcpy(target_area, method_name, method_name_length);
            target_area = (char*)target_area + method_name_length;
        }
        return (int32_t)total_data_size;
    }
    case PLUG_REQUEST_METHOD_PARAMS:  // Характеристики параметров некоторого метода, предоставляемого втыкалой для обращения.
    {
        if (!source_area || source_length < (int32_t)(sizeof(struct RequestMethodParams)))
            return (int32_t)PLUGIN_ERR_INVALID_SOURCE_FIELD;

        struct RequestMethodParams* request_params_ptr = (struct RequestMethodParams*)source_area;
        // Ищем элемент массива plugin_method_table для метода request_params_ptr->method_name. Перегруженных методов наша
        // втыкала не имеет, поэтому ординал метода игнорируем.
        const struct PluginMethodTable* method_desc = FindMethodDesc(request_params_ptr->method_name);
        if (!method_desc)
            return (int32_t)PLUGIN_ERR_METHOD_NOT_FOUND;    // Метод с затребованным именем не существует.

        // Дескриптор нужного метода найден. Копируем в приёмник сначала plugin_method_table[method_index].params_definer, а затем
        // plugin_method_table[method_index].params_definer.param_types_count элементов массива plugin_method_table[method_index].method_params.
        size_t target_data_size = sizeof(struct PluginMethodDefiner) + (size_t)(method_desc->params_definer.param_types_count) * sizeof(uint32_t);
        if ((int32_t)target_data_size > target_length)
            return (int32_t)PLUGIN_ERR_BUFFER_TOO_SMALL;    // Приёмный буфер слишком мал для всех данных дескриптора метода.
        // Копируем сначала тело дескриптора фиксированной структуры.
        memcpy(target_area, &(method_desc->params_definer), sizeof(struct PluginMethodDefiner));
        // А затем нужное количество элементов из plugin_method_table[method_index].method_params.
        memcpy((char*)target_area + sizeof(struct PluginMethodDefiner), method_desc->method_params,
               (size_t)(method_desc->params_definer.param_types_count) * sizeof(uint32_t));
        // Копирование данных на выход завершилось успешно. Как результат всей функции возвращаем длину скопированных данных.
        return (int32_t)target_data_size;
    }
    case PLUG_REQUEST_HELPER_FUNCTIONS:     // Указатели на служебные функции внешней среды.
    {
        if (source_length < (int32_t)(sizeof(struct PluginHelperFunctions)))
            return (int32_t)PLUGIN_ERR_BUFFER_TOO_SMALL;

        // Копируем в надлежащее место адреса служебных функций ядра.
        memcpy(&helper_funcs, source_area, sizeof(struct PluginHelperFunctions));
        return 0; // Ответ на такой запрос не формируется.
    }
    default:
        return (int32_t)PLUGIN_ERR_INVALID_REQUEST;
    }
}

// Её вызывная функция, служащая для обращения к методам класса втыкалы.
MYTHLON_MODULE_EXPORT void CallPluginMethod(const char* method_name, uintptr_t plugin_method_call_id)
{
    // Получение условного идента текущего экземпляра класса втыкалы, к которому обращён вызов.
    uintptr_t plugin_object_id = helper_funcs.get_instance_func(plugin_method_call_id);
    // Поиск этого экземпляра в связном списке - хранилище объектов-втыкал.
    struct FilePluginStatus* use_plugin_instance = FindPluginStatus(plugin_object_id);
    if (!use_plugin_instance)
    {  // Экземпляр объекта втыкалы, к методу которого обращён вызов, пока ещё не существует. Нужно его создать.
        use_plugin_instance = malloc(sizeof(struct FilePluginStatus));
        if (!use_plugin_instance)
        {
            helper_funcs.set_runtime_error_func(plugin_method_call_id, (uint32_t)THRM_MEMORY_ALLOCATION_ERROR, memory_allocation);
            return;
        }
        // Инициализируем его поля в исходное состояние.
        use_plugin_instance->external_object_id = plugin_object_id;
        memset(use_plugin_instance->filename, 0, ArraySize(use_plugin_instance->filename));
        memset(use_plugin_instance->filemode, 0, ArraySize(use_plugin_instance->filemode));
        use_plugin_instance->file_handle = NULL;
        use_plugin_instance->file_error = 0;
        // Непосредственно вставка в связный список.
        InsertNewPluginStatus(plugin_method_call_id, use_plugin_instance);
    }

    if (strcmp(method_name, plugin_destroy_method_name) == 0)
    { // Это метод-деструктор.
        // Сначала вызовем деструктор самого объекта для корректного завершения его собственных внутренних операций.
        MethodDestroy(plugin_method_call_id, use_plugin_instance);
        // Далее нужно удалить из связного списка элемент plugin_status, соответствующий удаляемому объекту втыкалы.
        DeletePluginStatus(plugin_method_call_id, use_plugin_instance);
        return;
    }

    // Для всех прочих методов выбираем исполнитель по таблице обработчиков plugin_method_table.
    const struct PluginMethodTable* method_desc = FindMethodDesc(method_name);
    if (method_desc)
        method_desc->method_func(plugin_method_call_id, use_plugin_instance);
    else
        helper_funcs.set_runtime_error_func(plugin_method_call_id, (uint32_t)THRM_METHOD_NOT_FOUND, method_not_found);
}

// Инициализирующий метод - конструктор.
void MethodInit(uintptr_t plugin_method_call_id, struct FilePluginStatus* plugin_status)
{
    if (helper_funcs.params_count_func(plugin_method_call_id) != 0)
        // Есть какие-то аргументы, то это попытка немедленно открыть указанный файл.
        MethodFileOpen(plugin_method_call_id, plugin_status);
}

// Застроковщик. Возвращает в виде строки текущее состояние объекта. Параметров не принимает.
#define MAX_STR_BUFFER_LENGTH 2048
void MethodStringize(uintptr_t plugin_method_call_id, struct FilePluginStatus* plugin_status)
{
    static char FILE_STRING[] = "Файл - ";
    static char FILE_MODE_STRING[] = " : Режим - ";
    static char FILE_ERROR_STRING[] = " : Ошибка - ";
    static char WRAPPER_STRING[] = " : Оболочка - ";

    if (helper_funcs.params_count_func(plugin_method_call_id) != 0)
    {  // Есть какие-то аргументы. Для нас это будет выступать как ошибка.
        helper_funcs.set_runtime_error_func(plugin_method_call_id, (uint32_t)THRM_INVALID_PARAMS_COUNT, method_has_no_params);
        return;
    }

    char str_buffer[MAX_STR_BUFFER_LENGTH];
    char* current_buffer_ptr = str_buffer;
    // Строка FILE_STRING.
    strcpy(current_buffer_ptr, FILE_STRING);
    current_buffer_ptr += ArraySize(FILE_STRING) - 1;
    // Имя файла.
    strcpy(current_buffer_ptr, plugin_status->filename);
    current_buffer_ptr += strlen(plugin_status->filename);
    // Строка FILE_MODE_STRING.
    strcpy(current_buffer_ptr, FILE_MODE_STRING);
    current_buffer_ptr += ArraySize(FILE_MODE_STRING) - 1;
    // Режим обращения к файлу.
    strcpy(current_buffer_ptr, plugin_status->filemode);
    current_buffer_ptr += strlen(plugin_status->filemode);
    // При наличии текущей ошибки работы с файлом расширим формируемую строку информацией о ней.
    if (plugin_status->file_error)
    {
        // Строка FILE_ERROR_STRING.
        strcpy(current_buffer_ptr, FILE_ERROR_STRING);
        current_buffer_ptr += ArraySize(FILE_ERROR_STRING) - 1;
        // Код ошибки.
        _itoa(plugin_status->file_error, current_buffer_ptr, 10);
        current_buffer_ptr += strlen(current_buffer_ptr);
    }
    // Наконец, присоединим к итоговой строке данные об иденте оберточного объекта.
    // Строка WRAPPER_STRING.
    strcpy(current_buffer_ptr, WRAPPER_STRING);
    current_buffer_ptr += ArraySize(WRAPPER_STRING) - 1;
    // Строковое представление идента оболочечного объекта верхнего уровня (существующего внутри исполнительской среды МУФЛОНА).
    ulltoa(plugin_status->external_object_id, current_buffer_ptr, 10);
    current_buffer_ptr += strlen(current_buffer_ptr);

    helper_funcs.set_result_value_func(plugin_method_call_id, (uint32_t)OBJECT_TYPE_STRING, str_buffer, (int32_t)(current_buffer_ptr - str_buffer));
}

void MethodDestroy(uintptr_t plugin_method_call_id, struct FilePluginStatus* plugin_status)
{ // Деструктор.
    // Закроем файл, если он был ранее открыт.
    if (plugin_status->file_handle)
        MethodFileClose(0, plugin_status);
}

// Прочие методы класса втыкалы общего назначения (собственно, производящие работу с файлом).
// Открытие файла. Имеет один либо два строковых аргумента - имя файла и режим работы с ним.
void MethodFileOpen(uintptr_t plugin_method_call_id, struct FilePluginStatus* plugin_status)
{
    int arg_count = helper_funcs.params_count_func(plugin_method_call_id);
    if (arg_count != 1 && arg_count != 2)
    { // Параметров не один и не два - ошибка.
        helper_funcs.set_runtime_error_func(plugin_method_call_id, (uint32_t)THRM_INVALID_PARAMS_COUNT, method_has_one_or_two_param);
        return;
    }
    if ((enum ObjectType)helper_funcs.param_type_func(plugin_method_call_id, 0) != OBJECT_TYPE_STRING)
    { // Параметр 0 - имя открываемого файла - не принадлежит к строковому типу.
        helper_funcs.set_runtime_error_func(plugin_method_call_id, (uint32_t)THRM_INVALID_PARAM_TYPE, filename_not_string);
        return;
    }
    int32_t filename_length = helper_funcs.param_string_size_func(plugin_method_call_id, 0);
    if (filename_length >= FILENAME_LENGTH)
    {
        helper_funcs.set_runtime_error_func(plugin_method_call_id, (uint32_t)THRM_INVALID_PARAM_LENGTH, string_too_long);
        return;
    }

    char filename[FILENAME_LENGTH];
    char filemode[FILEMODE_LENGTH];
    strcpy(filemode, "r");  // По умолчанию открываем файл только для чтения.
    helper_funcs.param_get_value_func(plugin_method_call_id, 0, filename, FILENAME_LENGTH);
    filename[filename_length] = 0;

    if (arg_count == 2)
    {
        if ((enum ObjectType)helper_funcs.param_type_func(plugin_method_call_id, 1) != OBJECT_TYPE_STRING)
        { // Параметр 1 - режим открытия файла - не принадлежит к строковому типу.
            helper_funcs.set_runtime_error_func(plugin_method_call_id, (uint32_t)THRM_INVALID_PARAM_TYPE, filemode_not_string);
            return;
        }
        int32_t filemode_length = helper_funcs.param_string_size_func(plugin_method_call_id, 1);
        if (filemode_length >= FILEMODE_LENGTH)
        {
            helper_funcs.set_runtime_error_func(plugin_method_call_id, (uint32_t)THRM_INVALID_PARAM_LENGTH, string_too_long);
            return;
        }
        helper_funcs.param_get_value_func(plugin_method_call_id, 1, filemode, FILEMODE_LENGTH);
        filemode[filemode_length] = 0;
    }

    if (!plugin_status->file_handle)
        MethodFileClose(0, plugin_status);
    errno = 0;
    plugin_status->file_handle = fopen(filename, filemode);
    plugin_status->file_error = errno;
    helper_funcs.set_result_value_func(plugin_method_call_id, (uint32_t)OBJECT_TYPE_INTEGER, &plugin_status->file_error,
                         (int32_t)sizeof(plugin_status->file_error));
}

// Закрытие открытого файла. Не имеет аргументов.
void MethodFileClose(uintptr_t plugin_method_call_id, struct FilePluginStatus* plugin_status)
{
    if (plugin_method_call_id)
    { // Возможен также внеконтекстуальный вызов это функции (как правило, из деструктора MethodDestroy()).
      // В этом случае plugin_method_call_id == 0 и наличие переданных параметров проверяться не будет.
        if (helper_funcs.params_count_func(plugin_method_call_id) != 0)
        {   // Есть какие-то аргументы. Для нас это будет выступать как ошибка.
            helper_funcs.set_runtime_error_func(plugin_method_call_id, (uint32_t)THRM_INVALID_PARAMS_COUNT, method_has_no_params);
            return;
        }
    }

    if (plugin_status->file_handle)
    {
        fclose(plugin_status->file_handle);
        plugin_status->file_handle = NULL;
    }
}

// Из программы на МУФЛОНЕ вызов выполняется так: files_object.read(int read_length).
// read_length - количество считываемых символов.
// Полученные символы возвращаются в виде неформатированной строки.
void MethodFileRead(uintptr_t plugin_method_call_id, struct FilePluginStatus* plugin_status)
{
    // Проверка корректности переданных параметров. Он должен быть единственным и численным.
    if (helper_funcs.params_count_func(plugin_method_call_id) != 1)
    { // Параметр не один - ошибка.
        helper_funcs.set_runtime_error_func(plugin_method_call_id, (uint32_t)THRM_INVALID_PARAMS_COUNT, method_has_one_param);
        return;
    }
    if (!IsArgumNumeric(plugin_method_call_id, 0, bytes_count_not_number))
        return;  // Единственный аргумент должен быть числовым (цело- или дробно-).

    if (!plugin_status->file_handle)
    {
        helper_funcs.set_runtime_error_func(plugin_method_call_id, (uint32_t)THRM_INVALID_PARAMS_COUNT, file_not_opened);
        return;
    }

    int try_read_length = GetUniIntNumber(plugin_method_call_id, 0);
    char* read_buffer = malloc(try_read_length);

    errno = 0;
    size_t fact_read_length = fread(read_buffer, 1, try_read_length, plugin_status->file_handle);
    plugin_status->file_error = errno;
    helper_funcs.set_result_value_func(plugin_method_call_id, (uint32_t)OBJECT_TYPE_STRING, read_buffer, (int32_t)fact_read_length);

    free(read_buffer);
}

// Общий вид вызова - files_object.write(var_type what_right).
// var_type - любой элементарный тип переменной - логический тип, строка, число произвольного типа.
// Запись в файл целого числа - files_object.write(111).
// Запись в файл дробного числа - files_object.write(3.1415925).
// Запись в файл текстовой строки - files_object.write("Third String").
void MethodFileWrite(uintptr_t plugin_method_call_id, struct FilePluginStatus* plugin_status)
{
    // Проверим допустимость входных параметров. Он должен быть единственным и может быть при этом любого типа.
    if (helper_funcs.params_count_func(plugin_method_call_id) != 1)
    { // Параметр не один - ошибка.
        helper_funcs.set_runtime_error_func(plugin_method_call_id, (uint32_t)THRM_INVALID_PARAMS_COUNT, method_has_one_param);
        return;
    }
    
    if (!plugin_status->file_handle)
    {
        helper_funcs.set_runtime_error_func(plugin_method_call_id, (uint32_t)THRM_INVALID_PARAMS_COUNT, file_not_opened);
        return;
    }

    int write_length = 0;
    switch ((enum ObjectType)helper_funcs.param_type_func(plugin_method_call_id, 0))
    {
        case OBJECT_TYPE_NONE:           // Пустой параметр (и, соответственно, контейнер) None.
            break;
        case OBJECT_TYPE_LOGICAL:        // Логическое значение bool.
            write_length = 1;
            break;
        case OBJECT_TYPE_SYMBOL:         // Одиночный символ char.
            write_length = 1;
            break;
        case OBJECT_TYPE_INTEGER:        // Целочисленный параметр int32_t.
            write_length = (int)sizeof(int32_t);
            break;
        case OBJECT_TYPE_DOUBLE:         // Число с плавающей точкой double.
            write_length = (int)sizeof(double);
            break;
        case OBJECT_TYPE_STRING:         // Символьная строка std::string.
            write_length = helper_funcs.param_string_size_func(plugin_method_call_id, 0);
            break;
        default:
            helper_funcs.set_runtime_error_func(plugin_method_call_id, (uint32_t)THRM_INVALID_PARAM_TYPE, type_not_supported);
            return;
    }
    // Сохраняем в файл write_length байт из входного буфера первого (с нулевым индексом) аргумента метода.
    void* medium_buffer = malloc(write_length);
    if (!medium_buffer)
    {
        helper_funcs.set_runtime_error_func(plugin_method_call_id, (uint32_t)THRM_MEMORY_ALLOCATION_ERROR, memory_allocation);
        return;
    }
    helper_funcs.param_get_value_func(plugin_method_call_id, 0, medium_buffer, write_length);

    errno = 0;
    int result = (int)fwrite(medium_buffer, (size_t)1, (size_t)write_length, plugin_status->file_handle);
    plugin_status->file_error = errno;
    free(medium_buffer);

    helper_funcs.set_result_value_func(plugin_method_call_id, (uint32_t)OBJECT_TYPE_INTEGER, &result, (int32_t)sizeof(result));
}

void MethodFileSeek(uintptr_t plugin_method_call_id, struct FilePluginStatus* plugin_status)
{
    // Проверка корректности переданных параметров. Их может быть один либо два и оба должны быть численными.
    int arg_count = helper_funcs.params_count_func(plugin_method_call_id);
    if (arg_count != 1 && arg_count != 2)
    { // Параметров не один и не два - ошибка.
        helper_funcs.set_runtime_error_func(plugin_method_call_id, (uint32_t)THRM_INVALID_PARAMS_COUNT, method_has_one_or_two_param);
        return;
    }
    if (!IsArgumNumeric(plugin_method_call_id, 0, target_point_not_number))
        return;  // Первый аргумент - целевая точка перемещения - должен быть числовым (цело- или дробно-).

    int target_point = GetUniIntNumber(plugin_method_call_id, 0);
    int seek_mode = SEEK_SET;

    if (arg_count == 2)
    {
        if (!IsArgumNumeric(plugin_method_call_id, 1, seekmode_not_number))
            return;  // Второй аргумент - режим перемещения - также должен быть числовым (цело- или дробно-).
        seek_mode = GetUniIntNumber(plugin_method_call_id, 1);
        if (seek_mode != SEEK_SET && seek_mode != SEEK_CUR && seek_mode != SEEK_END)
        {
            helper_funcs.set_runtime_error_func(plugin_method_call_id, (uint32_t)THRM_INVALID_PARAMS_COUNT, invalid_seekmode);
            return;
        }
    }

    if (!plugin_status->file_handle)
    {
        helper_funcs.set_runtime_error_func(plugin_method_call_id, (uint32_t)THRM_INVALID_PARAMS_COUNT, file_not_opened);
        return;
    }

    errno = 0;
    int result = fseek(plugin_status->file_handle, target_point, seek_mode);
    plugin_status->file_error = errno;
    helper_funcs.set_result_value_func(plugin_method_call_id, (uint32_t)OBJECT_TYPE_INTEGER, &result, (int32_t)sizeof(result));
}

void MethodFileTell(uintptr_t plugin_method_call_id, struct FilePluginStatus* plugin_status)
{
    if (helper_funcs.params_count_func(plugin_method_call_id) != 0)
    {   // Данный метод не принимает аргументов. Есть они всё же есть, то это ошибка.
        helper_funcs.set_runtime_error_func(plugin_method_call_id, (uint32_t)THRM_INVALID_PARAMS_COUNT, method_has_no_params);
        return;
    }

    if (!plugin_status->file_handle)
    {
        helper_funcs.set_runtime_error_func(plugin_method_call_id, (uint32_t)THRM_INVALID_PARAMS_COUNT, file_not_opened);
        return;
    }

    errno = 0;
    int result = ftell(plugin_status->file_handle);
    plugin_status->file_error = errno;
    helper_funcs.set_result_value_func(plugin_method_call_id, (uint32_t)OBJECT_TYPE_INTEGER, &result, (int32_t)sizeof(result));
}

void MethodFileRewind(uintptr_t plugin_method_call_id, struct FilePluginStatus* plugin_status)
{
    if (helper_funcs.params_count_func(plugin_method_call_id) != 0)
    {   // Данный метод не принимает аргументов. Есть они всё же есть, то это ошибка.
        helper_funcs.set_runtime_error_func(plugin_method_call_id, (uint32_t)THRM_INVALID_PARAMS_COUNT, method_has_no_params);
        return;
    }

    if (!plugin_status->file_handle)
    {
        helper_funcs.set_runtime_error_func(plugin_method_call_id, (uint32_t)THRM_INVALID_PARAMS_COUNT, file_not_opened);
        return;
    }

    errno = 0;
    rewind(plugin_status->file_handle);
    plugin_status->file_error = errno;
    helper_funcs.set_result_value_func(plugin_method_call_id, (uint32_t)OBJECT_TYPE_INTEGER,
                                       &plugin_status->file_error, (int32_t)sizeof(int));
}

void MethodFileIsOpen(uintptr_t plugin_method_call_id, struct FilePluginStatus* plugin_status)
{
    if (helper_funcs.params_count_func(plugin_method_call_id) != 0)
    {   // Данный метод не принимает аргументов. Есть они всё же есть, то это ошибка.
        helper_funcs.set_runtime_error_func(plugin_method_call_id, (uint32_t)THRM_INVALID_PARAMS_COUNT, method_has_no_params);
        return;
    }

    bool if_file_open = plugin_status->file_handle;
    helper_funcs.set_result_value_func(plugin_method_call_id, (uint32_t)OBJECT_TYPE_LOGICAL, &if_file_open, (int32_t)sizeof(if_file_open));
}

void MethodFileRemove(uintptr_t plugin_method_call_id, struct FilePluginStatus* plugin_status)
{
    // Проверка допустимости переданных в метод аргументов.
    if (helper_funcs.params_count_func(plugin_method_call_id) != 1)
    { // Проверка на количество переданных параметров. Он должен быть единственным.
        helper_funcs.set_runtime_error_func(plugin_method_call_id, (uint32_t)THRM_INVALID_PARAMS_COUNT, method_has_one_param);
        return;
    }
    if ((enum ObjectType)helper_funcs.param_type_func(plugin_method_call_id, 0) != OBJECT_TYPE_STRING)
    { // Единственный аргумент должен быть строковым.
        helper_funcs.set_runtime_error_func(plugin_method_call_id, (uint32_t)THRM_INVALID_PARAM_TYPE, filename_not_string);
        return;
    }
    int32_t filename_length = helper_funcs.param_string_size_func(plugin_method_call_id, 0);
    if (filename_length >= FILENAME_LENGTH)
    {
        helper_funcs.set_runtime_error_func(plugin_method_call_id, (uint32_t)THRM_INVALID_PARAM_LENGTH, string_too_long);
        return;
    }

    char remove_filename[FILENAME_LENGTH];
    helper_funcs.param_get_value_func(plugin_method_call_id, 0, remove_filename, FILENAME_LENGTH);
    remove_filename[filename_length] = 0;

    errno = 0;
    int result = remove(remove_filename);
    plugin_status->file_error = errno;
    helper_funcs.set_result_value_func(plugin_method_call_id, (uint32_t)OBJECT_TYPE_INTEGER, &result, (int32_t)sizeof(result));
}

void MethodFileRename(uintptr_t plugin_method_call_id, struct FilePluginStatus* plugin_status)
{
    // Сначала выполним проверку на наличие и допустимость входных аргументов метода.
    if (helper_funcs.params_count_func(plugin_method_call_id) != 2)
    {   // Данный метод принимает два аргумента - имя исходного файла и его желпемое имя. Есть это не так, это ошибка.
        helper_funcs.set_runtime_error_func(plugin_method_call_id, (uint32_t)THRM_INVALID_PARAMS_COUNT, method_has_two_param);
        return;
    }
    if ((enum ObjectType)helper_funcs.param_type_func(plugin_method_call_id, 0) != OBJECT_TYPE_STRING ||
        (enum ObjectType)helper_funcs.param_type_func(plugin_method_call_id, 1) != OBJECT_TYPE_STRING)
    { // Оба аргумента должны быть строковыми.
        helper_funcs.set_runtime_error_func(plugin_method_call_id, (uint32_t)THRM_INVALID_PARAM_TYPE, filename_not_string);
        return;
    }
    int32_t src_filename_length = helper_funcs.param_string_size_func(plugin_method_call_id, 0),
            dest_filename_length = helper_funcs.param_string_size_func(plugin_method_call_id, 1);
    if (src_filename_length >= FILENAME_LENGTH || dest_filename_length >= FILENAME_LENGTH)
    {
        helper_funcs.set_runtime_error_func(plugin_method_call_id, (uint32_t)THRM_INVALID_PARAM_LENGTH, string_too_long);
        return;
    }

    char src_renaming_filename[FILENAME_LENGTH];
    char dest_filename[FILENAME_LENGTH];
    helper_funcs.param_get_value_func(plugin_method_call_id, 0, src_renaming_filename, FILENAME_LENGTH);
    helper_funcs.param_get_value_func(plugin_method_call_id, 1, dest_filename, FILENAME_LENGTH);
    src_renaming_filename[src_filename_length] = 0;
    dest_filename[dest_filename_length] = 0;

    errno = 0;
    int result = rename(src_renaming_filename, dest_filename);
    plugin_status->file_error = errno;
    helper_funcs.set_result_value_func(plugin_method_call_id, (uint32_t)OBJECT_TYPE_INTEGER, &result, (int32_t)sizeof(result));
}

void MethodFileStatus(uintptr_t plugin_method_call_id, struct FilePluginStatus* plugin_status)
{
    if (helper_funcs.params_count_func(plugin_method_call_id) != 0)
    {   // Данный метод не принимает аргументов. Есть они всё же есть, то это ошибка.
        helper_funcs.set_runtime_error_func(plugin_method_call_id, (uint32_t)THRM_INVALID_PARAMS_COUNT, method_has_no_params);
        return;
    }
    helper_funcs.set_result_value_func(plugin_method_call_id, (uint32_t)OBJECT_TYPE_INTEGER, &plugin_status->file_error, (int32_t)sizeof(int));
}

void MethodFileEof(uintptr_t plugin_method_call_id, struct FilePluginStatus* plugin_status)
{
    if (helper_funcs.params_count_func(plugin_method_call_id) != 0)
    {   // Данный метод не принимает аргументов. Если они всё же есть, то для нас это будет выступать как ошибка.
        helper_funcs.set_runtime_error_func(plugin_method_call_id, (uint32_t)THRM_INVALID_PARAMS_COUNT, method_has_no_params);
        return;
    }

    bool eof_result = true;
    if (plugin_status->file_handle)
        eof_result = feof(plugin_status->file_handle);
    helper_funcs.set_result_value_func(plugin_method_call_id, (uint32_t)OBJECT_TYPE_LOGICAL, &eof_result, (int32_t)sizeof(eof_result));
}

void MethodFileError(uintptr_t plugin_method_call_id, struct FilePluginStatus* plugin_status)
{
    if (helper_funcs.params_count_func(plugin_method_call_id) != 0)
    {   // Данный метод не принимает аргументов. Если они всё же есть, то для нас это будет выступать как ошибка.
        helper_funcs.set_runtime_error_func(plugin_method_call_id, (uint32_t)THRM_INVALID_PARAMS_COUNT, method_has_no_params);
        return;
    }

    bool error_result = true;
    if (plugin_status->file_handle)
        error_result = ferror(plugin_status->file_handle);
    helper_funcs.set_result_value_func(plugin_method_call_id, (uint32_t)OBJECT_TYPE_LOGICAL, &error_result, (int32_t)sizeof(error_result));
}

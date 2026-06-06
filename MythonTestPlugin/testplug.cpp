
// Двоичное тестовое дополнение - "втыкало" - для интерпретатора МУФЛОН.  Служит для работы модульных тестов интерпретатора,
// проверяющих работоспособность механизма втыкал.

#include "pch.h"

#include "testplug.h"

#include <string>
#include <optional>
#include <fstream>
#include <cstdio>

using namespace std;

std::unordered_map<uintptr_t, PluginInstance> object_table;

constexpr const char plugin_init_method_name[] = PLUGIN_INIT_METHOD;
constexpr const char plugin_destroy_method_name[] = PLUGIN_DESTROY_METHOD;
constexpr const char plugin_str_function_name[] = PLUGIN_STR_FUNCTION_METHOD;
constexpr const char plugin_add_method_name[] = PLUGIN_ADD_METHOD;

constexpr double ZERO_TOLERANCE = 1E-9; // Предел, все числа ниже которого считаются нулём.

extern "C"
{
    MYTHLON_MODULE_EXPORT const char* GetPluginsInfoFunction(uint32_t load_level)
    {
        static constexpr const char* PLUGIN_INFO_FUNC_LIST[] =
        {
            "GetPluginInfo",    // Имя информирующей функции для единственной втыкалы этого модуля.
            ""                  // Обязательный пустой закрывающий элемент списка.
        };

        return PLUGIN_INFO_FUNC_LIST[0];
    }
    
    // Информирующая функция (информатор) для данной втыкалы.
    MYTHLON_MODULE_EXPORT int32_t GetPluginInfo(uint32_t request_type, void* source_area, int32_t source_length, void* target_area, int32_t target_length)
    {
        static constexpr const char PLUGIN_NAME[] = "TestPlugin";
        static constexpr const char EXECUTE_FUNCTION_NAME[] = "CallPluginMethod";
        static constexpr const char* PLUGIN_CLASS_METHODS_LIST[] =
        {   // Список существующих публичных методов класса втыкалы. Специальные методы (инициализатор, компаратор, конвертер в строку, и.т.д.)
            // сюда тоже можно включить, что мы и сделаем.
            plugin_init_method_name,      // Стандартное имя инициализирующего метода класса втыкалы (конструктора).
            plugin_str_function_name,     // Стандартное имя строкофикатора - преобразователя текущего состояния объекта втыкалы в строку.
            "add_all",
            "AddAll",
            "find_zero",
            "FindZero",
            "find_char",
            "FindChar",
            "ston",
            "Ston",
            "print_hello",
            "PrintHello",
            ""  // Обязательный пустой завершающий элемент списка.
        };

        switch (static_cast<PluginInfoRequest>(request_type))
        {
            case PluginInfoRequest::PLUG_REQUEST_PLUGIN_NAME:           // Получение имени втыкалы.
                if (target_length < static_cast<int32_t>(std::size(PLUGIN_NAME)))
                    return static_cast<int32_t>(PluginErrorCode::PLUGIN_ERR_BUFFER_TOO_SMALL);

                strcpy((char*)target_area, PLUGIN_NAME);
                return static_cast<int32_t>(std::size(PLUGIN_NAME));
            case PluginInfoRequest::PLUG_REQUEST_CALL_FUNCTION_NAME:    // Получение имени функции, выполняющей вызов методов данной втыкалы.
                if (target_length < static_cast<int32_t>(std::size(EXECUTE_FUNCTION_NAME)))
                    return static_cast<int32_t>(PluginErrorCode::PLUGIN_ERR_BUFFER_TOO_SMALL);

                strcpy((char*)target_area, EXECUTE_FUNCTION_NAME);
                return static_cast<int32_t>(std::size(EXECUTE_FUNCTION_NAME));
            case PluginInfoRequest::PLUG_REQUEST_METHOD_LIST:           // Список имён методов класса втыкалы, доступных для вызова.
            {
                // Рассчитаем полную длину списка, который нам нужно возвратить в ответ на данный запрос.
                size_t total_data_size = 0;
                for (const char* method_name : PLUGIN_CLASS_METHODS_LIST)
                    total_data_size += (strlen(method_name) + 1);
                if (target_length < total_data_size)
                    return static_cast<int32_t>(PluginErrorCode::PLUGIN_ERR_BUFFER_TOO_SMALL);

                // Длина приёмного буфера достаточна.
                for (const char* method_name : PLUGIN_CLASS_METHODS_LIST)
                {
                    size_t method_name_length = strlen(method_name) + 1;
                    memcpy(target_area, method_name, method_name_length);
                    target_area = reinterpret_cast<char*>(target_area) + method_name_length;
                }
                return static_cast<int32_t>(total_data_size);
            }
            case PluginInfoRequest::PLUG_REQUEST_METHOD_PARAMS:  // Характеристики параметров некоторого метода, предоставляемого втыкалой для обращения.
            {
                if (!source_area || source_length < static_cast<int32_t>(sizeof(RequestMethodParams)))
                    return PluginErrorCode::PLUGIN_ERR_INVALID_SOURCE_FIELD;

                RequestMethodParams* request_params_ptr = reinterpret_cast<RequestMethodParams*>(source_area);
                std::string req_method_name(request_params_ptr->method_name);
                // 
                std::pair<size_t, PluginInstance::CopyCharmResult> copy_result =
                    PluginInstance::CopyCharm(req_method_name, target_area, static_cast<size_t>(target_length));
                if (copy_result.second == PluginInstance::CopyCharmResult::COPY_CHARM_OK)
                { // Копирование данных на выход завершилось успешно. Как результат всей функции возвращаем длину скопированных данных.
                    return static_cast<int32_t>(copy_result.first);
                }
                else
                { // При копировании произошла какая-то ошибка. Транслируем ошибку типа PluginInstance::CopyCharmResult в ошибку класса
                  // PluginErrorCode, которую и возвращаем в качестве результата работы функции.
                    switch (copy_result.second)
                    {
                    case PluginInstance::CopyCharmResult::COPY_CHARM_METHOD_NOT_FOUND:
                        return static_cast<int32_t>(PluginErrorCode::PLUGIN_ERR_METHOD_NOT_FOUND);
                    case PluginInstance::CopyCharmResult::COPY_CHARM_BUFFER_TOO_SMALL:
                        return static_cast<int32_t>(PluginErrorCode::PLUGIN_ERR_BUFFER_TOO_SMALL);
                    default:
                        return static_cast<int32_t>(PluginErrorCode::PLUGIN_ERR_INVALID_REQUEST);
                    }
                }
            }
            case PluginInfoRequest::PLUG_REQUEST_HELPER_FUNCTIONS:
            {
                if (source_length < static_cast<int32_t>(sizeof(PluginHelperFunctions)))
                    return static_cast<int32_t>(PluginErrorCode::PLUGIN_ERR_BUFFER_TOO_SMALL);
                
                PluginHelperFunctions helper_functions;
                memcpy(&helper_functions, source_area, sizeof(PluginHelperFunctions));
                PluginInstance::SetHelperFunctions(helper_functions);
                return 0;   // Ответа на данный запрос не формируется.
            }
            default:
                return static_cast<int32_t>(PluginErrorCode::PLUGIN_ERR_INVALID_REQUEST);
        }
    }
    
    // Её вызывная функция, служащая для обращения к методам класса втыкалы.
    MYTHLON_MODULE_EXPORT void CallPluginMethod(const char* method_name, uintptr_t plugin_method_call_id)
    {
        // Получение условного идента текущего экземпляра класса втыкалы, к которому обращён вызов.
        uintptr_t plugin_object_id = PluginInstance::GetInstanceId()(plugin_method_call_id);
        // Поиск этого экземпляра в хэш-таблице - хранилище объектов-втыкал.
        std::unordered_map<uintptr_t, PluginInstance>::iterator object_table_it = object_table.find(plugin_object_id);
        if (object_table_it == object_table.end())
            // Экземпляр объекта втыкалы, к методу которого обращён вызов, пока ещё не существует. Нужно его создать.
            object_table_it = object_table.emplace(plugin_object_id, PluginInstance()).first;

        if (strcmp(method_name, plugin_destroy_method_name) == 0)
        { // Это метод-деструктор. Такой вызов не переадресуется объекту, а вместо этого объект просто уничтожается.
            object_table.erase(object_table_it);
            return;
        }

        // Для всех прочих методов они переадресуются внутренним функциям класса с помощью функции-члена Call().
        object_table_it->second.Call(method_name, plugin_method_call_id);
    }
}

const unordered_map<string_view, PluginInstance::PluginCallMethod> PluginInstance::plugin_method_table_
{
    // Специальные стандартные методы Муфлон-классов.
    {{plugin_init_method_name, std::size(plugin_init_method_name) - 1}, &PluginInstance::MethodInit},
    {{plugin_str_function_name, std::size(plugin_str_function_name) - 1}, &PluginInstance::MethodStringize},
    // Прочие свободно определяемые методы класса втыкалы.
    {"add_all"sv, &PluginInstance::MethodTestAddAll},
    {"AddAll"sv, &PluginInstance::MethodTestAddAll},
    {"find_zero"sv, &PluginInstance::MethodTestFindZero},
    {"FindZero"sv, &PluginInstance::MethodTestFindZero},
    {"find_char"sv, &PluginInstance::MethodTestFindChar},
    {"FindChar"sv, &PluginInstance::MethodTestFindChar},
    {"ston"sv, &PluginInstance::MethodTestSton},
    {"Ston"sv, &PluginInstance::MethodTestSton},
    {"print_hello"sv, &PluginInstance::MethodTestPrintHello},
    {"PrintHello"sv, &PluginInstance::MethodTestPrintHello}
};

const std::unordered_map<std::string_view, PluginInstance::ParamsCharm> PluginInstance::plugin_method_params_charm_
{
    // Стандартные специальные методы в нашем случае не принимают никаких параметров.
    {
        {plugin_init_method_name, std::size(plugin_init_method_name) - 1},
        {.params_definer = {.check_mode = MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL}}
    },
    {
        {plugin_str_function_name, std::size(plugin_str_function_name) - 1},
        {.params_definer = {.check_mode = MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL}}
    },
    //
    {"add_all"sv, {.params_definer = {.arg_count_min = 0, .arg_count_max = UINT_MAX}}},     // Допускается любое количество параметров любого типа.
    {"AddAll"sv, {.params_definer = {.arg_count_min = 0, .arg_count_max = UINT_MAX}}},      // Аналогично.
    {"find_zero"sv, {.params_definer = {.arg_count_min = 0, .arg_count_max = UINT_MAX}}},   // Аналогично.
    {"FindZero"sv, {.params_definer = {.arg_count_min = 0, .arg_count_max = UINT_MAX}}},    // Аналогично.
    {
        "find_char"sv,  // Строго два строковых аргумента.
        {.params_definer = {.arg_count_min = 2, .arg_count_max = 2, .check_mode = MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_EQUAL, .param_types_count = 2},
         .params_type = {MethodParamType::PARAM_TYPE_STRING, MethodParamType::PARAM_TYPE_STRING}}
    },
    {
        "FindChar"sv,  // Также точно два строковых аргумента.
        {.params_definer = {.arg_count_min = 2, .arg_count_max = 2, .check_mode = MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_EQUAL, .param_types_count = 2},
         .params_type = {MethodParamType::PARAM_TYPE_STRING, MethodParamType::PARAM_TYPE_STRING}}
    },
    {
        "ston"sv,   // Строго один строковый параметр.
        {.params_definer = {.arg_count_min = 1, .arg_count_max = 1, .check_mode = MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_EQUAL, .param_types_count = 1},
         .params_type = {MethodParamType::PARAM_TYPE_STRING}}
    },
    {
        "Ston"sv,   // Аналогично.
        {.params_definer = {.arg_count_min = 1, .arg_count_max = 1, .check_mode = MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_EQUAL, .param_types_count = 1},
         .params_type = {MethodParamType::PARAM_TYPE_STRING}}
    },
    {
        "print_hello"sv,    // Не принимает параметров.
        {.params_definer = {.check_mode = MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL}}
    },
    {
        "PrintHello"sv,     // Точно так же.
        {.params_definer = {.check_mode = MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL}}
    }
};

PluginHelperFunctions PluginInstance::helper_funcs_;

// Инициализирующий метод - конструктор. В нашем случае он параметров не принимает.
void PluginInstance::MethodInit(uintptr_t plugin_method_call_id)
{
    if (helper_funcs_.params_count_func(plugin_method_call_id) != 0)
        // Есть какие-то аргументы. Для нас это будет выступать как ошибка.
        helper_funcs_.set_runtime_error_func(plugin_method_call_id, static_cast<uint32_t>(ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT),
                              "Конструктор этого класса не должен иметь параметров");
}

// Застроковщик. Возвращает в виде строки текущее состояние объекта. Параметров не принимает.
void PluginInstance::MethodStringize(uintptr_t plugin_method_call_id)
{
    static constexpr const char TEST_PLUG_STRINGIZE_TEXT[] = "TestPlugin";

    if (helper_funcs_.params_count_func(plugin_method_call_id) != 0)
        // Есть какие-то аргументы. Для нас это будет выступать как ошибка.
        helper_funcs_.set_runtime_error_func(plugin_method_call_id, static_cast<uint32_t>(ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT),
                              "Строкофикатор не должен иметь параметров");
    else
        // Возвращаем строку, условно маркирующую сущность данного класса.
        helper_funcs_.set_result_value_func(plugin_method_call_id, static_cast<uint32_t>(ObjectType::OBJECT_TYPE_STRING),
                             (void*)(TEST_PLUG_STRINGIZE_TEXT), static_cast<int32_t>(std::size(TEST_PLUG_STRINGIZE_TEXT) - 1));
}

// Возвращает число (целое либо дробное), равное сумме всех аргументов функции. Суммируются числа или строки, содержащие числа,
// в любой комбинации. Аргументы любого другого типа вызывают ошибку времени исполнения.
void PluginInstance::MethodTestAddAll(uintptr_t plugin_method_call_id)
{
    int32_t int_result = 0;
    double double_result = 0.0;
    bool is_double_summ = false;
    bool is_arg_error = false;
    //
    constexpr size_t STR_BUFFER_SIZE = 128;
    char str_buffer[STR_BUFFER_SIZE];

    int32_t args_count = helper_funcs_.params_count_func(plugin_method_call_id);
    for (int32_t arg_index = 0; arg_index < args_count; ++arg_index)
    {
        ObjectType arg_type = static_cast<ObjectType>(helper_funcs_.param_type_func(plugin_method_call_id, arg_index));
        switch (arg_type)
        {
            case ObjectType::OBJECT_TYPE_LOGICAL:  // Логическое значение bool.
            {
                bool arg_bool;
                if (helper_funcs_.param_get_value_func(plugin_method_call_id, arg_index, &arg_bool, sizeof(bool)) < static_cast<int32_t>(sizeof(bool)))
                {   // Считать логический аргумент не получилось. Далее по флагу is_arg_error будет выставлена ошибка периода исполннения.
                    is_arg_error = true;
                    break;
                }
                // Очередной операнд для суммирования считан успешно в логическое поле arg_bool.
                if (is_double_summ)
                    double_result += static_cast<double>(arg_bool);
                else
                    int_result += static_cast<int32_t>(arg_bool);
                break;
            }
            case ObjectType::OBJECT_TYPE_INTEGER:  // Целочисленный параметр int32_t.
            {
                int32_t arg_intval;
                if (helper_funcs_.param_get_value_func(plugin_method_call_id, arg_index, &arg_intval, sizeof(int32_t)) < static_cast<int32_t>(sizeof(int32_t)))
                {   // Считать логический аргумент не получилось. Далее по флагу is_arg_error будет выставлена ошибка периода исполннения.
                    is_arg_error = true;
                    break;
                }
                // Очередной операнд для суммирования считан успешно в целочисленное поле arg_intval.
                if (is_double_summ)
                    double_result += static_cast<double>(arg_intval);
                else
                    int_result += arg_intval;
                break;
            }
            case ObjectType::OBJECT_TYPE_DOUBLE:   // Число с плавающей точкой double.
            {
                double arg_doubleval;
                if (helper_funcs_.param_get_value_func(plugin_method_call_id, arg_index, &arg_doubleval, sizeof(double)) < static_cast<int32_t>(sizeof(double)))
                {   // Считать логический аргумент не получилось. Далее по флагу is_arg_error будет выставлена ошибка периода исполннения.
                    is_arg_error = true;
                    break;
                }
                // Очередной операнд для суммирования считан успешно в поле числа двойной точности arg_doubleval.
                if (!is_double_summ)
                { // Если очередной аргумент дробный, а сумма ранее вычислялась, как целая, то переводим её расчёт в "дробный" режим.
                    double_result = static_cast<double>(int_result);
                    is_double_summ = true;
                }
                double_result += arg_doubleval;
                break;
            }
            case ObjectType::OBJECT_TYPE_STRING:   // Символьная строка std::string.
            {
                int32_t arg_string_size = helper_funcs_.param_string_size_func(plugin_method_call_id, arg_index);
                if (arg_string_size >= STR_BUFFER_SIZE)
                {   // Слишком длинная строка с заведомо недопустимым содержимым.
                    is_arg_error = true;
                    break;
                }

                if (helper_funcs_.param_get_value_func(plugin_method_call_id, arg_index, str_buffer, arg_string_size) != arg_string_size)
                {   // Какая-то проблема с извлечением строки.
                    is_arg_error = true;
                    break;
                }
                str_buffer[arg_string_size] = 0;    // Превращаем строку в C-строку путём вставки оконечного нуля.

                char* val_end_pos;
                int32_t arg_intval = strtol(str_buffer, &val_end_pos, 10);
                if (val_end_pos - str_buffer != arg_string_size)
                { // Это не целое число (полностью преобразовать его как целое у нас не получилось). Попробуем разобрать его как дробное.
                    double arg_doubleval = strtod(str_buffer, &val_end_pos);
                    if (val_end_pos - str_buffer == arg_string_size)
                    { // Это корректное дробное число.
                        if (!is_double_summ)
                        { // Если очередной аргумент дробный, а сумма ранее вычислялась, как целая, то переводим её расчёт в "дробный" режим.
                            double_result = static_cast<double>(int_result);
                            is_double_summ = true;
                        }
                        double_result += arg_doubleval;
                    }
                    else
                    { // Полагать аргумент верным дробным числом тоже, увы, не получается. Возвращаем ошибку.
                        helper_funcs_.set_runtime_error_func(plugin_method_call_id, static_cast<uint32_t>(ThrowMessageNumber::THRM_INVALID_PARAM_VALUE),
                                              "Строка не содержит корректного числа");
                        return;
                    }
                }
                else
                { // Это целое число.
                    if (is_double_summ)
                        double_result += static_cast<double>(arg_intval);
                    else
                        int_result += arg_intval;
                }
                break;
            }
            default:    // Неподдерживаемый тип аргумента.
            {
                helper_funcs_.set_runtime_error_func(plugin_method_call_id, static_cast<uint32_t>(ThrowMessageNumber::THRM_INVALID_PARAM_TYPE),
                                      "Суммирование значений такого типа не поддерживается");
                return;
            }
        }

        if (is_arg_error)
        { // Произошёл сбой при получении очередного аргумента функции.
            helper_funcs_.set_runtime_error_func(plugin_method_call_id, static_cast<uint32_t>(ThrowMessageNumber::THRM_INVALID_PARAM_VALUE),
                                  "Сбой при извлечении аргумента");
            return;
        }
    }
        
    if (is_double_summ)
        helper_funcs_.set_result_value_func(plugin_method_call_id, static_cast<uint32_t>(ObjectType::OBJECT_TYPE_DOUBLE),
                             &double_result, static_cast<int32_t>(sizeof(double)));
    else
        helper_funcs_.set_result_value_func(plugin_method_call_id, static_cast<uint32_t>(ObjectType::OBJECT_TYPE_INTEGER),
                             &int_result, static_cast<int32_t>(sizeof(int32_t)));
}

// Ищет первый встретившийся нуль в последовательности аргументов функции. Возвращает индекс первого найденного нуля.
// Анализируются только числа - целые либо дробные. Встреча фактического аргумента любого другого типа трактуется как ошибка.
void PluginInstance::MethodTestFindZero(uintptr_t plugin_method_call_id)
{
    bool is_arg_error = false;
    bool is_zero_found = false;

    int32_t args_count = helper_funcs_.params_count_func(plugin_method_call_id);
    for (int32_t arg_index = 0; arg_index < args_count; ++arg_index)
    {
        ObjectType arg_type = static_cast<ObjectType>(helper_funcs_.param_type_func(plugin_method_call_id, arg_index));
        switch (arg_type)
        {
            case ObjectType::OBJECT_TYPE_LOGICAL:  // Логическое значение bool.
            {
                bool arg_bool;
                if (helper_funcs_.param_get_value_func(plugin_method_call_id, arg_index, &arg_bool, sizeof(bool)) < static_cast<int32_t>(sizeof(bool)))
                {   // Считать логический аргумент не получилось. Далее по флагу is_arg_error будет выставлена ошибка периода исполннения.
                    is_arg_error = true;
                    break;
                }
                // Очередной операнд для суммирования считан успешно в логическое поле arg_bool.
                if (!arg_bool)
                    is_zero_found = true;
                break;
            }
            case ObjectType::OBJECT_TYPE_INTEGER:  // Целочисленный параметр int32_t.
            {
                int32_t arg_intval;
                if (helper_funcs_.param_get_value_func(plugin_method_call_id, arg_index, &arg_intval, sizeof(int32_t)) < static_cast<int32_t>(sizeof(int32_t)))
                {   // Считать логический аргумент не получилось. Далее по флагу is_arg_error будет выставлена ошибка периода исполннения.
                    is_arg_error = true;
                    break;
                }
                // Очередной операнд для суммирования считан успешно в целочисленное поле arg_intval.
                if (arg_intval == 0)
                    is_zero_found = true;
                break;
            }
            case ObjectType::OBJECT_TYPE_DOUBLE:   // Число с плавающей точкой double.
            {
                double arg_doubleval;
                if (helper_funcs_.param_get_value_func(plugin_method_call_id, arg_index, &arg_doubleval, sizeof(double)) < static_cast<int32_t>(sizeof(double)))
                {   // Считать логический аргумент не получилось. Далее по флагу is_arg_error будет выставлена ошибка периода исполннения.
                    is_arg_error = true;
                    break;
                }
                // Очередной операнд для суммирования считан успешно в поле числа двойной точности arg_doubleval.
                if (fabs(arg_doubleval) < ZERO_TOLERANCE)
                    is_zero_found = true;
                break;
            }
            default:     // Неподдерживаемый тип аргумента.
            {
                helper_funcs_.set_runtime_error_func(plugin_method_call_id, static_cast<uint32_t>(ThrowMessageNumber::THRM_INVALID_PARAM_TYPE),
                                      "Поиск нуля среди значений такого типа не поддерживается");
                return;
            }
        }

        if (is_arg_error)
        { // Произошёл сбой при получении очередного аргумента функции.
            helper_funcs_.set_runtime_error_func(plugin_method_call_id, static_cast<uint32_t>(ThrowMessageNumber::THRM_INVALID_PARAM_VALUE),
                                  "Сбой при извлечении аргумента");
            return;
        }

        if (is_zero_found)
        {
            helper_funcs_.set_result_value_func(plugin_method_call_id, static_cast<uint32_t>(ObjectType::OBJECT_TYPE_INTEGER),
                                 &arg_index, static_cast<int32_t>(sizeof(int32_t)));
            return;
        }
    }

    // Нулевого члена в последовательности аргументов не найдено. Возвращаем -1.
    int32_t arg_index = -1;
    helper_funcs_.set_result_value_func(plugin_method_call_id, static_cast<uint32_t>(ObjectType::OBJECT_TYPE_INTEGER),
                         &arg_index, static_cast<int32_t>(sizeof(int32_t)));
}

// Функция имеет два строковых формальных аргумента. Выполняет поиск положения первой буквы (символа) второй строки в первой. Возвращает целое
// число, содержащее номер этой позиции, либо std::numeric_limits<int>::max(), если такого символа в первом аргументе не содержится.
// Нестроковые аргументы порождают ошибку исполнения.
void PluginInstance::MethodTestFindChar(uintptr_t plugin_method_call_id)
{
    if (helper_funcs_.params_count_func(plugin_method_call_id) != 2)
    {
        helper_funcs_.set_runtime_error_func(plugin_method_call_id, static_cast<uint32_t>(ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT),
                              "Метод принимает строго два строковых параметра");
        return;
    }

    if (helper_funcs_.param_type_func(plugin_method_call_id, 0) != static_cast<int32_t>(ObjectType::OBJECT_TYPE_STRING) ||
        helper_funcs_.param_type_func(plugin_method_call_id, 1) != static_cast<int32_t>(ObjectType::OBJECT_TYPE_STRING))
    {
        helper_funcs_.set_runtime_error_func(plugin_method_call_id, static_cast<uint32_t>(ThrowMessageNumber::THRM_INVALID_PARAM_TYPE),
                              "Метод принимает только строковые параметры");
        return;
    }

    int32_t arg_0_string_size = helper_funcs_.param_string_size_func(plugin_method_call_id, 0);
    int32_t arg_1_string_size = helper_funcs_.param_string_size_func(plugin_method_call_id, 1);
    if (arg_0_string_size < 0 || arg_1_string_size < 0)
    {
        helper_funcs_.set_runtime_error_func(plugin_method_call_id, static_cast<uint32_t>(ThrowMessageNumber::THRM_INVALID_PARAM_VALUE),
                              "Сбой при извлечении длины параметров");
        return;
    }

    std::unique_ptr<std::string> arg_0_ptr(std::make_unique<std::string>(arg_0_string_size, '\0'));
    std::unique_ptr<std::string> arg_1_ptr(std::make_unique<std::string>(arg_1_string_size, '\0'));
    if (helper_funcs_.param_get_value_func(plugin_method_call_id, 0, arg_0_ptr->data(), arg_0_string_size) != arg_0_string_size ||
        helper_funcs_.param_get_value_func(plugin_method_call_id, 1, arg_1_ptr->data(), arg_1_string_size) != arg_1_string_size)
    {
        helper_funcs_.set_runtime_error_func(plugin_method_call_id, static_cast<uint32_t>(ThrowMessageNumber::THRM_INVALID_PARAM_VALUE),
                              "Сбой при извлечении содержимого параметров");
        return;
    }

    size_t char_position = std::string::npos;
    if (!arg_1_ptr->empty())
        char_position = (*arg_0_ptr).find((*arg_1_ptr)[0]);
    
    // -1 - значение, возвращаемое при неуспешном поиске.
    int32_t int_char_position = char_position != std::string::npos ? static_cast<int32_t>(char_position) : -1;

    helper_funcs_.set_result_value_func(plugin_method_call_id, static_cast<uint32_t>(ObjectType::OBJECT_TYPE_INTEGER),
                         &int_char_position, static_cast<int32_t>(sizeof(int32_t)));
}

// Функция ston(string_argument) - преобразование строки в число (целое либо дробное с плавающей точкой). Нестроковый аргумент -
// - возврат ошибки периода исполнения.
void PluginInstance::MethodTestSton(uintptr_t plugin_method_call_id)
{
    if (helper_funcs_.params_count_func(plugin_method_call_id) != 1)
    {
        helper_funcs_.set_runtime_error_func(plugin_method_call_id, static_cast<uint32_t>(ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT),
                              "Метод принимает строго один строковый параметр");
        return;
    }

    if (helper_funcs_.param_type_func(plugin_method_call_id, 0) != static_cast<int32_t>(ObjectType::OBJECT_TYPE_STRING))
    {
        helper_funcs_.set_runtime_error_func(plugin_method_call_id, static_cast<uint32_t>(ThrowMessageNumber::THRM_INVALID_PARAM_TYPE),
                              "Метод принимает только строковый параметр");
        return;
    }

    int32_t arg_0_string_size = helper_funcs_.param_string_size_func(plugin_method_call_id, 0);
    if (arg_0_string_size < 0)
    {
        helper_funcs_.set_runtime_error_func(plugin_method_call_id, static_cast<uint32_t>(ThrowMessageNumber::THRM_INVALID_PARAM_VALUE),
                              "Сбой при извлечении длины параметров");
        return;
    }

    std::unique_ptr<std::string> arg_0_ptr(std::make_unique<std::string>(arg_0_string_size, '\0'));
    if (helper_funcs_.param_get_value_func(plugin_method_call_id, 0, arg_0_ptr->data(), arg_0_string_size) != arg_0_string_size)
    {
        helper_funcs_.set_runtime_error_func(plugin_method_call_id, static_cast<uint32_t>(ThrowMessageNumber::THRM_INVALID_PARAM_VALUE),
                              "Сбой при извлечении содержимого параметров");
        return;
    }

    // Сначала преобразуем строку как целое число.
    const char* arg_0_value_src = arg_0_ptr->c_str();
    char* int_value_end_pos;
    long long_result = strtol(arg_0_value_src, &int_value_end_pos, 10);
    size_t int_result_len = int_value_end_pos - arg_0_value_src;

    // Далее попробуем преобразовать строку как дробное число с плавающей точкой.
    char* double_value_end_pos;
    double double_result = strtod(arg_0_value_src, &double_value_end_pos);
    size_t double_result_len = double_value_end_pos - arg_0_value_src;
    // Выберем тот тип значения, который использовал все символы исходника.
    if (int_result_len == arg_0_ptr->size())
    {
        if (long_result < INT32_MIN || long_result > INT32_MAX)
        {
            // Строка кодирует целое число, но за пределами типа int32_t.
            helper_funcs_.set_runtime_error_func(plugin_method_call_id, static_cast<uint32_t>(ThrowMessageNumber::THRM_OVERFLOW),
                                  "Целочисленное переполнение");
        }
        else
        {
            int32_t int_result = static_cast<int32_t>(long_result);
            helper_funcs_.set_result_value_func(plugin_method_call_id, static_cast<uint32_t>(ObjectType::OBJECT_TYPE_INTEGER),
                                 &int_result, static_cast<int32_t>(sizeof(int32_t)));
        }
    }
    else if (double_result_len == arg_0_ptr->size())
    {
        helper_funcs_.set_result_value_func(plugin_method_call_id, static_cast<uint32_t>(ObjectType::OBJECT_TYPE_DOUBLE),
                             &double_result, static_cast<int32_t>(sizeof(double)));
    }
    else
    {
        // Обе функции преобразования строки в число (strtol() и strtod()) не могут преобразовать весь исходник в число.
        helper_funcs_.set_runtime_error_func(plugin_method_call_id, static_cast<uint32_t>(ThrowMessageNumber::THRM_INVALID_PARAM_VALUE),
                              "Строка не содержит корректного числа");
    }
}

// Функция не принимает аргументов, печатает и возвращает как результат строку "Hello".
void PluginInstance::MethodTestPrintHello(uintptr_t plugin_method_call_id)
{
    if (helper_funcs_.params_count_func(plugin_method_call_id) != 0)
    {
        helper_funcs_.set_runtime_error_func(plugin_method_call_id, static_cast<uint32_t>(ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT),
                              "Метод не принимает никаких параметров");
        return;
    }

    std::string hello_string = "Hello"s;
    if (helper_funcs_.print_to_context_func(plugin_method_call_id, static_cast<uint32_t>(ObjectType::OBJECT_TYPE_STRING),
                             hello_string.data(), static_cast<int32_t>(hello_string.size())) != static_cast<int32_t>(hello_string.size()))
    {
        helper_funcs_.set_runtime_error_func(plugin_method_call_id, static_cast<uint32_t>(ThrowMessageNumber::THRM_CONTEXT_OUT_FAIL),
                              "Печать данных в контекст завершилась неудачно");
        return;
    }

    helper_funcs_.set_result_value_func(plugin_method_call_id, static_cast<uint32_t>(ObjectType::OBJECT_TYPE_STRING),
                         hello_string.data(), static_cast<int32_t>(hello_string.size()));
}

void PluginInstance::Call(const char* method_name, uintptr_t plugin_method_call_id)
{
    if (plugin_method_table_.count(method_name))
        return (this->*plugin_method_table_.at(method_name))(plugin_method_call_id);
    else
        helper_funcs_.set_runtime_error_func(plugin_method_call_id, static_cast<uint32_t>(ThrowMessageNumber::THRM_METHOD_NOT_FOUND), "Метод не найден");
}

std::pair<size_t, PluginInstance::CopyCharmResult> PluginInstance::CopyCharm
    (const std::string& req_method_name, void* target_area, size_t target_area_size)
{
    if (PluginInstance::plugin_method_params_charm_.count(req_method_name))
    {
        const ParamsCharm& req_method_charm = plugin_method_params_charm_.at(req_method_name);
        size_t total_data_length =
            sizeof(PluginMethodDefiner) + static_cast<size_t>(req_method_charm.params_definer.param_types_count) * sizeof(uint32_t);
        if (target_area_size < total_data_length)
            return {0, CopyCharmResult::COPY_CHARM_BUFFER_TOO_SMALL};

        // Места в приёмном буфере достаточно. Переносим туда сначала блок фиксированной структуры PluginMethodDefiner, а затем элементы
        // массива MethodParamType.
        memcpy(target_area, &req_method_charm.params_definer, sizeof(PluginMethodDefiner));
        // method_params_count_real - действительное количество типовых описателей аргументов, которые нужно перенести в приёмную область
        // из массива req_method_charm.params_type.
        size_t method_params_count_real =
            min(static_cast<size_t>(req_method_charm.params_definer.param_types_count), req_method_charm.params_type.size());
        // Переносим на выход основную часть элементов MethodParamType из массива req_method_charm.params_type.
        target_area = reinterpret_cast<char*>(target_area) + sizeof(PluginMethodDefiner);
        memcpy(target_area, req_method_charm.params_type.data(), method_params_count_real * sizeof(uint32_t));
        // Если в req_method_charm.params_type элементов почему-то не хватает (хотя такого быть никогда и не должно), дополним приёмную
        // область заполнителями.
        if (method_params_count_real < static_cast<size_t>(req_method_charm.params_definer.param_types_count))
        {
            target_area = reinterpret_cast<char*>(target_area) + method_params_count_real * sizeof(uint32_t);
            uint32_t params_type_filler = static_cast<uint32_t>(MethodParamType::PARAM_TYPE_ANY);
            while (method_params_count_real < static_cast<size_t>(req_method_charm.params_definer.param_types_count))
            {
                memcpy(target_area, &params_type_filler, sizeof(uint32_t));
                target_area = reinterpret_cast<char*>(target_area) + sizeof(uint32_t);
                ++method_params_count_real;
            }
        }
        return {total_data_length, CopyCharmResult ::COPY_CHARM_OK};
    }
    else
    {
        return {0, CopyCharmResult::COPY_CHARM_METHOD_NOT_FOUND};
    }
}


#include "plugin_caller.h"
#include "error_classes.h"
#include <unordered_set>
#include <optional>
#include <fstream>
#include <cstdio>

using namespace std;
using namespace runtime;

static constexpr char ERROR_WITHOUT_TEXT[] = "Нет текста";

// Вспомогательная функция проверки удовлетворения C-строкой правила предельной длины.
bool CheckStringMaxLength(const char* test_string, size_t max_length)
{
    size_t current_length = 0;
    for (; *(test_string + current_length) != 0 && current_length <= max_length; ++current_length);
    return current_length <= max_length;
}

// Структура, описывающая один конкретный вызов некоторого метода какой-либо отдельной втыкалы. Такая запись создаётся для каждого такого
// вызова в начале работы с ним (перед его исполнением), а удаляется после его полного завершения и обработки возвращенного им результата.
struct FullPluginMethodCallDefiner
{
    PluginInstance* plugin_instance;                // Указатель на экземпляр втыкалы, метод которой вызван.
    const ast::MethodDefiner* method = nullptr;     // Указатель на описание вызванного метода.
    ProgramCommandDescriptor call_command;          // Описание строки программы, в которой совершён данный вызов метода.
    Context* context = nullptr;                     // Указатель на контекст исполнения вызова.

    bool operator==(const FullPluginMethodCallDefiner& other) const
    {
        return plugin_instance == other.plugin_instance &&
               method == other.method &&
               call_command == other.call_command &&
               context == other.context;
    }

    bool operator!=(const FullPluginMethodCallDefiner& other) const
    {
        return !(*this == other);
    }
};

namespace std
{
    template<>
    struct hash<FullPluginMethodCallDefiner>
    {
        static constexpr size_t HASH_MULTIPLICATOR = 43; // Простое число, удалённое от степени 2.
        static constexpr size_t HASH_MULTIPLICATOR_2 = HASH_MULTIPLICATOR * HASH_MULTIPLICATOR;
        static constexpr size_t HASH_MULTIPLICATOR_3 = HASH_MULTIPLICATOR_2 * HASH_MULTIPLICATOR;
        static constexpr size_t HASH_MULTIPLICATOR_4 = HASH_MULTIPLICATOR_3 * HASH_MULTIPLICATOR;

        size_t operator()(const FullPluginMethodCallDefiner& hash_definer) const noexcept
        {
            return std::hash<PluginInstance*>{}(hash_definer.plugin_instance) +
                   std::hash<const ast::MethodDefiner*>{}(hash_definer.method) * HASH_MULTIPLICATOR +
                   std::hash<int>{}(hash_definer.call_command.module_id) * HASH_MULTIPLICATOR_2 +
                   std::hash<int>{}(hash_definer.call_command.module_string_number) * HASH_MULTIPLICATOR_3 +
                   std::hash<Context*>{}(hash_definer.context) * HASH_MULTIPLICATOR_4;
        }
    };
}

// Мультимножество, хранящее записи описания активных (пока ещё выполняющихся или не полностью завершённых) вызовов методов втыкал.
// Используется именно мультимножество для возможности хранить информацию о параллельно исполняющихся запросах, выполненных одной и
// той же строкой кода в параллельных потоках. Пока это просто резерв на будущее, так как в настощее время интерпретатор не поддерживает
// параллелизм.
std::unordered_multiset<FullPluginMethodCallDefiner> call_definers;

// Хранители символов (входных параметров, а также выходных значений и ошибок), связанных с каким-либо активным вызовом некоторого метода
// определённой втыкалы.
std::unordered_map<const FullPluginMethodCallDefiner*, std::vector<runtime::ObjectHolder>> plug_params;
std::unordered_map<const FullPluginMethodCallDefiner*, runtime::ObjectHolder> plug_retvals;
std::unordered_map<const FullPluginMethodCallDefiner*, runtime::RuntimeError> plug_errors;

extern "C"
{
    // Функция возвращает идент (фактически, указатель) на экземпляр(объект)-оболочку класса PluginInstance, оборачивающий тот экземпляр класса
    // втыкалы, к которому направлен вызов plugin_method_call_id.
    HELPERS_EXPORT_IMPORT uintptr_t PluginGetInstanceId(uintptr_t plugin_method_call_id)
    {
        FullPluginMethodCallDefiner* call_definer = reinterpret_cast<FullPluginMethodCallDefiner*>(plugin_method_call_id);
        if (!call_definer || plug_errors.count(call_definer) == 0)
            return PluginErrorCode::PLUGIN_ERR_INVALID_METHOD_CALL_ID;  // Неизвестный идентификатор вызова метода.

        return reinterpret_cast<uintptr_t>(call_definer->plugin_instance);
    }

    // Функция экспортируется ядром, импортируется втыкалой и вызывается изнутри её методов для передачи исполнительской среде информации
    // о том, что работа текущего метода завершилась событием, которое после его завершения должно привести к выбросу исключения msg_num
    // с сообщением except_text.
    MYTHLON_KERNEL_EXPORT int32_t PluginSetRuntimeError(uintptr_t plugin_method_call_id, uint32_t msg_num, const char* except_text)
    {
        FullPluginMethodCallDefiner* call_definer = reinterpret_cast<FullPluginMethodCallDefiner*>(plugin_method_call_id);
        if (!call_definer || plug_errors.count(call_definer) == 0)
            return PluginErrorCode::PLUGIN_ERR_INVALID_METHOD_CALL_ID;  // Неизвестный идентификатор вызова метода.
        if (msg_num > static_cast<uint32_t>(ThrowMessageNumber::THRM_MAX_VALUE))
            return PluginErrorCode::PLUGIN_ERR_INCORRECT_RUNTIME_ERROR; // Недопустимый тип ошибки.

        if (!except_text)
            except_text = ERROR_WITHOUT_TEXT;
        plug_errors[call_definer] =
            CreateErrorObject(call_definer->context->GetLastCommandDesc(), static_cast<ThrowMessageNumber>(msg_num), except_text);
        return PluginErrorCode::PLUGIN_ERR_NONE;   // Код нормального завершения.
    }

    MYTHLON_KERNEL_EXPORT int32_t PluginSetResultValue(uintptr_t plugin_method_call_id, uint32_t result_type, void* source_field, int32_t source_length)
    { // Функция установки значения, возвращаемого вызванным методом втыкалы. Значение считывается из поля source_field длиной не более source_length.
      // Возвращаемое значение указывает истинное количество считанных байт (при успешном выполнеии запроса) либо код ошибки (при её возникновении).
        if (!source_field)
            return PluginErrorCode::PLUGIN_ERR_INVALID_SOURCE_FIELD;    // Не указан буфер-источник возвращаемого значения.
        if (source_length < 0)
            return PluginErrorCode::PLUGIN_ERR_BUFFER_TOO_SMALL;
        FullPluginMethodCallDefiner* full_call_definer = reinterpret_cast<FullPluginMethodCallDefiner*>(plugin_method_call_id);
        if (!full_call_definer || plug_retvals.count(full_call_definer) == 0)
            return PluginErrorCode::PLUGIN_ERR_INVALID_METHOD_CALL_ID;  // Неизвестный идентификатор вызова метода.

        ObjectHolder& retval_holder = plug_retvals[full_call_definer];
        switch (static_cast<ObjectTypes>(result_type))
        {
        case ObjectTypes::OBJECT_TYPE_NONE:
            return 0;
        case ObjectTypes::OBJECT_TYPE_LOGICAL:
            if (source_length >= sizeof(bool))
            {
                retval_holder = ObjectHolder::Own(runtime::Bool(*reinterpret_cast<bool*>(source_field)));
                return sizeof(bool);
            }
            else
            {
                return PluginErrorCode::PLUGIN_ERR_BUFFER_TOO_SMALL;
            }
        case ObjectTypes::OBJECT_TYPE_INTEGER:
            if (source_length >= sizeof(int))
            {
                retval_holder = ObjectHolder::Own(runtime::Number(*reinterpret_cast<int*>(source_field)));
                return sizeof(int);
            }
            else
            {
                return PluginErrorCode::PLUGIN_ERR_BUFFER_TOO_SMALL;
            }
        case ObjectTypes::OBJECT_TYPE_DOUBLE:
            if (source_length >= sizeof(double))
            {
                retval_holder = ObjectHolder::Own(runtime::Number(*reinterpret_cast<double*>(source_field)));
                return sizeof(double);
            }
            else
            {
                return PluginErrorCode::PLUGIN_ERR_BUFFER_TOO_SMALL;
            }
        case ObjectTypes::OBJECT_TYPE_STRING:
            retval_holder = ObjectHolder::Own(runtime::String(std::string(reinterpret_cast<char*>(source_field), source_length)));
            return source_length;
        case ObjectTypes::OBJECT_TYPE_SYMBOL:
            if (source_length >= sizeof(char))
            {
                retval_holder = ObjectHolder::Own(runtime::String(std::string(1, *reinterpret_cast<char*>(source_field))));
                return sizeof(char);
            }
            else
            {
                return PluginErrorCode::PLUGIN_ERR_BUFFER_TOO_SMALL;
            }
        default:
            return PluginErrorCode::PLUGIN_ERR_UNSUPPORTED_TYPE;
        }
    }

    MYTHLON_KERNEL_EXPORT int32_t PluginParamsCount(uintptr_t plugin_method_call_id)
    { // Экспортируемая функция возвращает вызвавшему её методу втыкалы количество параданных в неё параметров.
        FullPluginMethodCallDefiner* call_definer = reinterpret_cast<FullPluginMethodCallDefiner*>(plugin_method_call_id);
        if (!call_definer || plug_params.count(call_definer) == 0)
            return PluginErrorCode::PLUGIN_ERR_INVALID_METHOD_CALL_ID;  // Неизвестный идентификатор вызова метода.

        return static_cast<int32_t>(plug_params[call_definer].size());
    }

    MYTHLON_KERNEL_EXPORT int32_t PluginParamType(uintptr_t plugin_method_call_id, uint32_t arg_number)
    { // Данная экспортируемая ядром функция сообщает вызывающему коду тип входного аргумента с номером arg_number.
        FullPluginMethodCallDefiner* full_call_definer = reinterpret_cast<FullPluginMethodCallDefiner*>(plugin_method_call_id);
        if (!full_call_definer || plug_params.count(full_call_definer) == 0)
            return PluginErrorCode::PLUGIN_ERR_INVALID_METHOD_CALL_ID;  // Неизвестный идентификатор вызова метода.
        uint32_t plug_params_count =
            static_cast<uint32_t>(plug_params[full_call_definer].size());
        if (arg_number >= plug_params_count)
            return PluginErrorCode::PLUGIN_ERR_INVALID_ARGUMENT_INDEX;  // Индекс аргумента за пределами допустимого.

        ObjectHolder& selected_param = plug_params[full_call_definer][arg_number];
        if (!selected_param)
            return static_cast<uint32_t>(ObjectTypes::OBJECT_TYPE_NONE);    // В контейнере хранится значение None.
        else if (selected_param.TryAs<runtime::Bool>())
            return static_cast<uint32_t>(ObjectTypes::OBJECT_TYPE_LOGICAL); // В контейнере находится логическое значение.
        else if (runtime::Number* number_val_ptr = selected_param.TryAs<runtime::Number>())
        { // Какое-то число, целое или с плавающей точкой.
            if (std::holds_alternative<int>(number_val_ptr->GetValue()))
                return static_cast<uint32_t>(ObjectTypes::OBJECT_TYPE_INTEGER);     // Это целое число.
            else if (std::holds_alternative<double>(number_val_ptr->GetValue()))
                return static_cast<uint32_t>(ObjectTypes::OBJECT_TYPE_DOUBLE);      // Число с плавающей точкой.
            else
                return static_cast<uint32_t>(ObjectTypes::OBJECT_TYPE_OTHER);
        }
        else if (selected_param.TryAs<runtime::String>())
            return static_cast<uint32_t>(ObjectTypes::OBJECT_TYPE_STRING);  // Строка.
        else
            return static_cast<uint32_t>(ObjectTypes::OBJECT_TYPE_OTHER);
    }

    MYTHLON_KERNEL_EXPORT int32_t PluginParamStringSize(uintptr_t plugin_method_call_id, uint32_t arg_number)
    { // Функция возвращает длину строки аргумента с номером arg_number, если данный аргумент является строковым.
        FullPluginMethodCallDefiner* full_call_definer = reinterpret_cast<FullPluginMethodCallDefiner*>(plugin_method_call_id);
        if (!full_call_definer || plug_params.count(full_call_definer) == 0)
            return PluginErrorCode::PLUGIN_ERR_INVALID_METHOD_CALL_ID;  // Неизвестный идентификатор вызова метода.

        uint32_t plug_params_count =
            static_cast<uint32_t>(plug_params[full_call_definer].size());
        if (arg_number >= plug_params_count)
            return PluginErrorCode::PLUGIN_ERR_INVALID_ARGUMENT_INDEX;  // Индекс аргумента за пределами допустимого.

        if (runtime::String* string_val_ptr = plug_params[full_call_definer][arg_number].TryAs<runtime::String>())
            return static_cast<int32_t>(string_val_ptr->SizeOf());
        else
            return PluginErrorCode::PLUGIN_ERR_IT_IS_NOT_STRING;   // Это не строка.
    }

    MYTHLON_KERNEL_EXPORT int32_t PluginParamGetValue(uintptr_t plugin_method_call_id, uint32_t arg_number, void* target_field, int32_t target_length)
    { // Функция копирует содержимое параметра arg_number в поле-приёмник, на которое указывает target_field. Предполагается, что
      // места там не менее, чем target_length байт. Возвращаемое значение - количество скопированных байт.
        if (!target_field)
            return PluginErrorCode::PLUGIN_ERR_INVALID_TARGET_FIELD;    // Не указан буфер-приемник получаемого значения.
        if (target_length < 0)
            return PluginErrorCode::PLUGIN_ERR_BUFFER_TOO_SMALL;

        FullPluginMethodCallDefiner* full_call_definer = reinterpret_cast<FullPluginMethodCallDefiner*>(plugin_method_call_id);
        if (!full_call_definer || plug_params.count(full_call_definer) == 0)
            return PluginErrorCode::PLUGIN_ERR_INVALID_METHOD_CALL_ID;  // Неизвестный идентификатор вызова метода.

        if (arg_number >= static_cast<uint32_t>(plug_params[full_call_definer].size()))
            return PluginErrorCode::PLUGIN_ERR_INVALID_ARGUMENT_INDEX;  // Индекс аргумента за пределами допустимого.
        ObjectHolder& selected_param = plug_params[full_call_definer][arg_number];

        if (!selected_param)
        { // Значение None.
            return 0;
        }
        else if (runtime::Bool* bool_ptr = selected_param.TryAs<runtime::Bool>())
        { // Однобайтовое логическое значение.
            if (target_length >= sizeof(bool))
            {
                *reinterpret_cast<bool*>(target_field) = bool_ptr->GetValue();
                return sizeof(bool);
            }
            else
            {
                return PluginErrorCode::PLUGIN_ERR_BUFFER_TOO_SMALL;
            }
        }
        else if (runtime::Number* number_val_ptr = selected_param.TryAs<runtime::Number>())
        { // Некоторое число.
            if (std::holds_alternative<int>(number_val_ptr->GetValue()))
            { // Целое.
                if (target_length >= sizeof(int))
                {
                    *reinterpret_cast<int*>(target_field) = std::get<int>(number_val_ptr->GetValue());
                    return sizeof(int);
                }
                else
                {
                    return PluginErrorCode::PLUGIN_ERR_BUFFER_TOO_SMALL;
                }
            }
            else if (std::holds_alternative<double>(number_val_ptr->GetValue()))
            { // Дробное с плавающей точкой.
                if (target_length >= sizeof(double))
                {
                    *reinterpret_cast<double*>(target_field) = std::get<double>(number_val_ptr->GetValue());
                    return sizeof(double);
                }
                else
                {
                    return PluginErrorCode::PLUGIN_ERR_BUFFER_TOO_SMALL;
                }
            }
            else
            {
                return PluginErrorCode::PLUGIN_ERR_UNSUPPORTED_TYPE;
            }
        }
        else if (runtime::String* string_ptr = selected_param.TryAs<runtime::String>())
        {
            const std::string& param_str_value = string_ptr->GetValue();
            size_t move_length = min(param_str_value.size(), static_cast<size_t>(target_length));
            memmove(target_field, param_str_value.data(), move_length);
            return static_cast<uint32_t>(move_length);
        }
        else
        {
            return PluginErrorCode::PLUGIN_ERR_UNSUPPORTED_TYPE;
        }
    }

    HELPERS_EXPORT_IMPORT int32_t PluginPrintToContext(uintptr_t plugin_method_call_id, uint32_t source_type, void* source_field, int32_t source_length)
    { // Функция направления данных типа source_type из буфера (source_field, source_length) в выходной поток, связанный с контекстом, который, в свою очередь,
      // ассоциирован с вызовом plugin_method_call_id.
        FullPluginMethodCallDefiner* full_call_definer = reinterpret_cast<FullPluginMethodCallDefiner*>(plugin_method_call_id);
        if (!full_call_definer || plug_params.count(full_call_definer) == 0)
            return PluginErrorCode::PLUGIN_ERR_INVALID_METHOD_CALL_ID;  // Неизвестный идентификатор вызова метода.

        switch (static_cast<ObjectTypes>(source_type))
        {
        case ObjectTypes::OBJECT_TYPE_NONE:
            return 0;   // Тип None - ничего не считываем и ничего не отправляем в поток контекста.
        case ObjectTypes::OBJECT_TYPE_LOGICAL:
            if (source_field && source_length >= sizeof(bool))
            {
                full_call_definer->context->GetOutputStream() << *reinterpret_cast<bool*>(source_field);
                return sizeof(bool);
            }
            else
            {
                return PluginErrorCode::PLUGIN_ERR_BUFFER_TOO_SMALL;
            }
        case ObjectTypes::OBJECT_TYPE_INTEGER:
            if (source_field && source_length >= sizeof(int))
            {
                full_call_definer->context->GetOutputStream() << *reinterpret_cast<int*>(source_field);
                return sizeof(int);
            }
            else
            {
                return PluginErrorCode::PLUGIN_ERR_BUFFER_TOO_SMALL;
            }
        case ObjectTypes::OBJECT_TYPE_DOUBLE:
            if (source_field && source_length >= sizeof(double))
            {
                full_call_definer->context->GetOutputStream() << *reinterpret_cast<double*>(source_field);
                return sizeof(double);
            }
            else
            {
                return PluginErrorCode::PLUGIN_ERR_BUFFER_TOO_SMALL;
            }
        case ObjectTypes::OBJECT_TYPE_STRING:
            if (source_field)
            {
                full_call_definer->context->GetOutputStream() << std::string(reinterpret_cast<char*>(source_field), source_length);
                return source_length;
            }
            else
            {
                return PluginErrorCode::PLUGIN_ERR_INVALID_SOURCE_FIELD;
            }
        case ObjectTypes::OBJECT_TYPE_SYMBOL:
            if (source_field && source_length >= sizeof(char))
            {
                full_call_definer->context->GetOutputStream() << *reinterpret_cast<char*>(source_field);
                return sizeof(char);
            }
            else
            {
                return PluginErrorCode::PLUGIN_ERR_BUFFER_TOO_SMALL;
            }
        default:
            return PluginErrorCode::PLUGIN_ERR_UNSUPPORTED_TYPE;
        }
    }
}

namespace ast
{
    static const std::string USE_INIT_METHOD_NAME(PLUGIN_INIT_METHOD);

    NewPluginInstance::NewPluginInstance
        (const std::string& class_name, std::vector<std::unique_ptr<Statement>>&& expression_args, const PluginDescData& plugin_desc) :
            class_name_(class_name), expression_args_(move(expression_args)), plugin_desc_(plugin_desc)
    {
        // Уже на этапе синтаксического анализа попытаемся выполнить некоторый предварительный контроль соответствия формата декларации,
        // создающей экземпляр класса данной втыкалы, требованиям, которые предъявляются самой втыкалой к составу формальных параметров
        // своего конструктора. Так как истинные типы фактических параметров будут нам известны только в процессе исполнения, пока всё,
        // что мы можем проверить прямо в процессе разбора - это их имеющееся количество, которое должно подходить к требованиям какого-либо
        // предусморенного втыкалой конструктора.
        if (!plugin_desc_.methods.count(USE_INIT_METHOD_NAME))
        { // Никаких конструкторов нет вовсе. В этом случае expression_args_ должен быть пустым.
            if (expression_args_.size())
                throw ParseError(ThrowMessageNumber::THRM_OBJECT_CTOR_HAS_NO_PARAMS);
        }
        else
        { // Какие-то конструкторы есть. Проверим возможность приёма ими expression_args_.size() параметров.
            if (GetMethodResult init_check_result = GetMethod(plugin_desc_, USE_INIT_METHOD_NAME, expression_args_.size());
                !init_check_result.method_definer)
                // Конструктора, принимающего expression_args_.size() параметров, не обнаружено.
                // Выбросим исключение с наиболее подходящим кодом ошибки, возвращенном в init_check_result при поиске.
                throw ParseError(init_check_result.err_num);
        }
    }

    ObjectHolder NewPluginInstance::Execute(Closure& closure, Context& context)
    {
        PrepareExecute(this, closure, context);

        ObjectHolder plugin_holder = ObjectHolder::Own(std::move(PluginInstance(class_name_, plugin_desc_, context)));
        PluginInstance* plugin_object = plugin_holder.TryAs<PluginInstance>();
        // Вычислим фактические значения аргументов, которые требуется передать конструктору.
        std::vector<ObjectHolder> actual_args;
        for (auto& cur_param_ptr : expression_args_)
            actual_args.push_back(cur_param_ptr->Execute(closure, context));

        if (GetMethodResult init_check_result = GetMethod(plugin_desc_, USE_INIT_METHOD_NAME, actual_args);
            init_check_result.method_definer)
            // Существует метод конструктора объекта втыкалы, принимающий аргументы actual_args.
            plugin_object->Call(USE_INIT_METHOD_NAME, actual_args, context);
        else   // Перегрузки конструктивного метода, способного принять аргументы actual_args, у втыкалы не оказалось.
            ThrowRuntimeError(context, init_check_result.err_num, init_check_result.err_text);

        return plugin_holder;
    }

    std::unique_ptr<Statement> CreateNewPluginInstance
        (const std::string& class_name, const PluginDescData& plugin_desc, std::vector<std::unique_ptr<Statement>> args)
    {
        return make_unique<NewPluginInstance>(NewPluginInstance(class_name, move(args), plugin_desc));
    }
} // namespace ast

namespace runtime
{
    std::string GenMethodNotFoundErrMess(const std::string& method_name)
    {
        return method_name + " - " + ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_METHOD_NOT_FOUND);
    }

    std::string GenMethodParamsErrMess(const std::string& method_name, size_t arg_count_act, size_t arg_count_min, size_t arg_count_max)
    {
        if (arg_count_min == arg_count_max)
        {
            if (arg_count_act != arg_count_min)
                return ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_METHOD) + method_name +
                    ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_DEMAND_EQUAL) +
                    std::to_string(arg_count_min) + ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_ARGUMENTS);
        }
        else
        {
            if (arg_count_act < arg_count_min)
            {
                return ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_METHOD) + method_name +
                    ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_DEMAND_GREATER_OR_EQUAL) +
                    std::to_string(arg_count_min) + ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_ARGUMENTS);
            }
            else if (arg_count_act > arg_count_max)
            {
                return ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_METHOD) + method_name +
                    ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_DEMAND_LESS_OR_EQUAL) +
                    std::to_string(arg_count_max) + ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_ARGUMENTS);
            }
        }
        return {};
    }

    std::pair<ThrowMessageNumber, std::string> CheckActualParamLegality(const ast::MethodDefiner& method_def, const std::vector<ObjectHolder>& actual_args)
    {
        static constexpr int PARAM_CHECK_QUANTITY_MASK = 3;

        // Сначала, если это требуется полем check_mode определителя вызываемого метода, проверим количественное соответствие между
        // actual_args и требованиями к количеству фактическаих параметров, предъявляемых вызываемым методом втыкалы.
        if (method_def.check_mode & PARAM_CHECK_QUANTITY_MASK)
        { // Проверка на количественное соответствие формальных и фактических параметров нужна.
            if (method_def.arg_count_min == method_def.arg_count_max)
            { // При этих условиях проводим проверку на точное соответствие числа затребованных и действительных фактических параметров.
                if (actual_args.size() != method_def.arg_count_min) // Наличное число фактических параметров неверное (не соответствует требованиям).
                    return {ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT,
                            GenMethodParamsErrMess(method_def.name, actual_args.size(), method_def.arg_count_min, method_def.arg_count_max)};
            }
            // При неравных величинах method_def.arg_count_min и method_def.arg_count_max применяем другие способы количественной проверки.
            else if (actual_args.size() > method_def.arg_count_max)
                return {ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT,
                        GenMethodParamsErrMess(method_def.name, actual_args.size(), method_def.arg_count_min, method_def.arg_count_max)};
            else if (actual_args.size() < method_def.arg_count_min)
                return {ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT,
                        GenMethodParamsErrMess(method_def.name, actual_args.size(), method_def.arg_count_min, method_def.arg_count_max)};
        }

        // Далее, если нужно, проверим типовое соответствие фактических параметров требованиям метода втыкалы.
        if (method_def.check_mode & MethodParamCheckMode::PARAM_CHECK_TYPE)
        {
            size_t i = 1;
            bool is_throw_exception = false;
            for (auto& current_param : actual_args)
            {
                if (i > method_def.param_types.size())
                    break;
                MethodParamType param_type = method_def.param_types[i - 1];
                if (current_param)
                {
                    if (current_param.TryAs<Number>() && !(param_type & MethodParamType::PARAM_TYPE_NUMERIC))
                        is_throw_exception = true;
                    if (current_param.TryAs<String>() && !(param_type & MethodParamType::PARAM_TYPE_STRING))
                        is_throw_exception = true;
                    if (current_param.TryAs<Bool>() && !(param_type & MethodParamType::PARAM_TYPE_LOGICAL))
                        is_throw_exception = true;
                }
                else
                {
                    is_throw_exception = !(param_type & MethodParamType::PARAM_TYPE_NONE);
                }

                if (is_throw_exception)
                {
                    std::string err_mess = ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_PARAMETER) + to_string(i) +
                        ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_OF_METHOD) + method_def.name +
                        ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_HAVE_INCOMPATIBLE_TYPE);
                    return {ThrowMessageNumber::THRM_PARAMS_TYPE_INCONSISTENCY, std::move(err_mess)};
                }
                ++i;
            }
        }
        // Аргументы удовлетворяют требованиям данной перегрузки метода.
        return {ThrowMessageNumber::THRM_UNKNOWN, {}};
    }

    // Грубый поиск подходящей перегрузки метода с именем method_name только по количеству формальных параметров.
    GetMethodResult GetMethod(const ast::PluginDescData& plugin_desc, const std::string& method_name, size_t arg_count)
    {
        ThrowMessageNumber err_num = ThrowMessageNumber::THRM_METHOD_NOT_FOUND;
        std::string err_mess = GenMethodNotFoundErrMess(method_name);

        auto methods_range_pair = plugin_desc.methods.equal_range(method_name);
        if (methods_range_pair.first != methods_range_pair.second)
        { // Какие-то методы с искомым именем класс втыкалы предоставляет. Так что проблемы могут быть только с количеством параметров.
            err_num = ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT;
            err_mess.clear();
        }

        for (auto& scan_method = methods_range_pair.first; scan_method != methods_range_pair.second; ++scan_method)
        { // Перебор всех имеющихся методов втыкалы с именем method_name в поисках того, который сможет принять arg_count параметров.
            if (arg_count >= scan_method->second.arg_count_min && arg_count <= scan_method->second.arg_count_max)
                return &methods_range_pair.first->second;  // Подходящий метод method_name, способный принять arg_count параметров, найден.
            // Рассматрвиаемый метод не подошёл. Выберем для него корректное сообщение об ошибке.
            err_mess = GenMethodParamsErrMess(method_name, arg_count, scan_method->second.arg_count_min, scan_method->second.arg_count_max);
        }
        // Подходящий по имени и количеству формальных параметров метод не найден.
        return {err_num, std::move(err_mess)};
    }

    // Более точный метод поиска перегрузки метода method_name с учётом истинных типов фактических параметров actual_args.
    GetMethodResult GetMethod(const ast::PluginDescData& plugin_desc, const std::string& method_name, const std::vector<ObjectHolder>& actual_args)
    {
        std::pair<ThrowMessageNumber, std::string> check_result{ThrowMessageNumber::THRM_METHOD_NOT_FOUND, GenMethodNotFoundErrMess(method_name)};
        size_t arg_count = actual_args.size();
        auto methods_range_pair = plugin_desc.methods.equal_range(method_name);
        if (methods_range_pair.first != methods_range_pair.second)
        {
            check_result.first = ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT;
            check_result.second = GenMethodParamsErrMess
                (method_name, arg_count, methods_range_pair.first->second.arg_count_min, methods_range_pair.first->second.arg_count_max);
        }

        for (auto& scan_method = methods_range_pair.first; scan_method != methods_range_pair.second; ++scan_method)
        { // Перебор всех имеющихся методов втыкалы с именем method_name в поисках того, который сможет принять аргументы actual_args.
            if (arg_count >= scan_method->second.arg_count_min && arg_count <= scan_method->second.arg_count_max)
            { // Подходящий метод method_name, способный принять arg_count параметров, найден.
                check_result = CheckActualParamLegality(scan_method->second, actual_args);
                if (check_result.first == ThrowMessageNumber::THRM_UNKNOWN)
                    return &methods_range_pair.first->second;   // Найдена перегрузка метода, подходящая как по количеству, так и по качеству аргументов.
            }
        }
        // Не одной пригодной перегрузки метода method_name, способной принять параметры actual_args, так и не обнаружено.
        return {check_result.first, check_result.second};
    }

    PluginInstance::PluginInstance(const std::string& class_name, const ast::PluginDescData& plugin_desc, Context& context) :
        class_name_(class_name), plugin_desc_(plugin_desc), context_(context)
    {}

    PluginInstance::PluginInstance(PluginInstance&& other) noexcept :
        class_name_(std::move(other.class_name_)), plugin_desc_(other.plugin_desc_), context_(other.context_)
    {
        other.class_name_.clear();
    }

    PluginInstance::~PluginInstance()
    {
        // Если втыкала опеределяет специальный метод PLUGIN_DESTROY_METHOD, то он будет использоваться как внутренний её деструктор (будет вызываться
        // при разрушении объекта).
        if (!class_name_.empty() && HasMethod(PLUGIN_DESTROY_METHOD, 0))
            Call(PLUGIN_DESTROY_METHOD, {}, context_);
    }

    void PluginInstance::Print(std::ostream& os, Context& context)
    {
        // Если втыкала определяет метод PLUGIN_STR_FUNCTION_METHOD, то для печати состояния класса используем именно его результат.
        if (HasMethod(PLUGIN_STR_FUNCTION_METHOD, 0))
            Call(PLUGIN_STR_FUNCTION_METHOD, {}, context)->Print(os, context);
        else
            os << class_name_ << " - " << (void*)this;
    }

    ObjectHolder PluginInstance::Call(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                      Context& context, const std::string& parent_name)
    {
        GetMethodResult method_check_result = GetMethod(plugin_desc_, method, actual_args);
        if (method_check_result.method_definer)
        {
            // Вызов метода втыкалы состоит из следующих шагов. Создание записи-определителя данного вызова,
            // подготовка аргументов - передача их в ассоциативное хранилице входных аргументов, непосредственно вызов,
            // обработка выходного возвращённого методом результата.
            auto definer_it = call_definers.emplace
                (FullPluginMethodCallDefiner{.plugin_instance = this, .method = method_check_result.method_definer,
                                             .call_command = context.GetLastCommandDesc(), .context = &context});
            plug_params[&(*definer_it)] = actual_args;
            plug_retvals[&(*definer_it)] = runtime::ObjectHolder();
            plug_errors[&(*definer_it)] = runtime::RuntimeError();
            struct DeleteDefiner
            { // Небольшой сторожок (типа ScopeGuard), который удалит все реквизиты, связанные с определителем совершаемого
              // вызова (*definer_it), когда они уже будут нам не нужны.
                DeleteDefiner(decltype(definer_it)& p_definer_it) : m_definer_it(p_definer_it)
                {}
               
                ~DeleteDefiner()
                { // При выходе сторожка из области видимости уничтожаем элемент definer_it описания вызова и все связанные с
                  // ним реквизиты - процедура вызова закончилась, они нам более не нужны.
                    plug_params.erase(&(*m_definer_it));
                    plug_retvals.erase(&(*m_definer_it));
                    plug_errors.erase(&(*m_definer_it));
                    call_definers.erase(m_definer_it);
                }
                
                decltype(definer_it)& m_definer_it;
            } delete_definer(definer_it);

            // Все параметры и приёмники результата подготовлены. А теперь сам вызов метода.
            plugin_desc_.call_func(method.c_str(), reinterpret_cast<uintptr_t>(&(*definer_it)));
            // Обрабатываем установленный методом втыкалы в definer_it выходной результат своей работы - значение или ошибку.
            runtime::ObjectHolder& plug_retval = plug_retvals[&(*definer_it)];
            runtime::RuntimeError& plug_error = plug_errors[&(*definer_it)];
            if (plug_error)
                // Метод выставил ошибку исполнения. Выбросим её в виде исключения.
                throw plug_error;
            else
                // Метод возвратил нормальный результат. Вернём его как итог работы данной функции.
                return plug_retvals[&(*definer_it)];
        }
        else
        {
            ThrowRuntimeError(context, method_check_result.err_num, method_check_result.err_text);
        }
    }
    
    bool PluginInstance::HasMethod(const std::string& method_name, size_t argument_count) const
    {
        return GetMethod(plugin_desc_, method_name, argument_count).method_definer;
    }
} // namespace runtime

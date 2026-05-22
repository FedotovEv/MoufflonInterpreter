#include "plugin_helpers.h"
#include "error_classes.h"

intptr_t ObjectHolderHandler(runtime::ObjectHolder& object_holder)
{
    return (intptr_t)(&object_holder);
}

intptr_t ContextHanlder(runtime::Context& context)
{
    return (intptr_t)(&context);
}

/*
extern "C"
{
    MYTHLON_KERNEL_EXPORT [[noreturn]] void ThrowRuntimeError(intptr_t context, int msg_num, const char* except_text)
    {

    }
}
*/

#include <optional>
#include <fstream>
#include <cstdio>

using namespace std;
using namespace runtime;
using namespace std;

namespace ast
{
    NewPluginInstance::NewPluginInstance
        (std::string&& class_name, std::vector<std::unique_ptr<Statement>>&& expression_args, PluginMethodList&& methods_def_list) :
         class_name_(move(class_name)), expression_args_(move(expression_args)), plugin_methods_list_(move(methods_def_list))
    {
        // Уже на этапе синтаксического анализа попытаемся выполнить некоторый предварительный контроль соответствия формата декларации,
        // создающей экземпляр класса данной втыкалы, требованиям, которые предъявляются самой втыкалой к составу формальных параметров
        // своего конструктора. Так как истинные типы фактических параметров будут нам известны только в процессе исполнения, пока всё,
        // что мы можем проверить прямо в процессе разбора - это их имеющееся количество, которое должно подходить к требованиям какого-либо
        // предусморенного втыкалой конструктора.
        if (!plugin_methods_list_.count(INIT_METHOD))
        { // Никаких конструкторов нет вовсе. В этом случае expression_args_ должен быть пустым.
            if (expression_args_.size())
                throw ParseError(ThrowMessageNumber::THRM_OBJECT_CTOR_HAS_NO_PARAMS);
        }
        else
        { // Какие-то конструкторы есть. Проверим возможность приёма ими expression_args_.size() параметров.
            auto init_methods_range_pair = plugin_methods_list_.equal_range(INIT_METHOD);
            size_t min_init_params = UINT_MAX, max_init_params = 0;
            for (auto scan_method_it = init_methods_range_pair.first; scan_method_it != init_methods_range_pair.second; ++scan_method_it)
            {
                min_init_params = min(min_init_params, scan_method_it->second.arg_count_min);
                max_init_params = max(max_init_params, scan_method_it->second.arg_count_max);
                if (expression_args_.size() >= scan_method_it->second.arg_count_min &&
                    expression_args_.size() <= scan_method_it->second.arg_count_max)
                    return; // Подходящий конструктор найден.
            }
            // Конструктора, принимающего expression_args_.size() параметров, не обнаружено.
            // Выбросим исключение с наиболее подходящим кодом ошибки.
            if (min_init_params == max_init_params)
                throw ParseError(ThrowMessageNumber::THRM_DEMAND_EQUAL);
            else if (expression_args_.size() > max_init_params)
                throw ParseError(ThrowMessageNumber::THRM_DEMAND_LESS_OR_EQUAL);
            else
                throw ParseError(ThrowMessageNumber::THRM_DEMAND_GREATER_OR_EQUAL);
        }
    }

    ObjectHolder NewPluginInstance::Execute(Closure& closure, Context& context)
    {
        PrepareExecute(this, closure, context);

        ObjectHolder plugin_holder = ObjectHolder::Own(PluginInstance(class_name_, plugin_methods_list_));
        PluginInstance* plugin_object = plugin_holder.TryAs<PluginInstance>();

        if (const ast::MethodDefiner* init_method_def = GetMethod(plugin_methods_list_, INIT_METHOD, expression_args_.size()))
        { // Существует метод конструктора объекта втыкалы, принимающий expression_args_.size() аргументов. Вызовем его сейчас,
          // вычислив и передав ему в качестве фактических параметров текущие значения actual_args.
            std::vector<ObjectHolder> actual_args;
            for (auto& cur_param_ptr : expression_args_)
                actual_args.push_back(cur_param_ptr->Execute(closure, context));
            plugin_object->Call(INIT_METHOD, actual_args, context);
        }
        return plugin_holder;
    }
} // namespace ast

namespace runtime
{
    const ast::MethodDefiner* GetMethod(const ast::PluginMethodList& plugin_methods_list, const std::string& method_name, size_t arg_count)
    {
        auto methods_range_pair = plugin_methods_list.equal_range(method_name);
        for (auto scan_method_it = methods_range_pair.first; scan_method_it != methods_range_pair.second; ++scan_method_it)
        {
            if (arg_count >= scan_method_it->second.arg_count_min && arg_count <= scan_method_it->second.arg_count_max)
                return &scan_method_it->second;
        }

        return nullptr; // Подходящий по имени и количеству формальных параметров метод не найден.
    }

    void CheckActualParamLegality(Context& context, const ast::MethodDefiner& method_def, const std::vector<ObjectHolder>& actual_args)
    {
        static constexpr int PARAM_CHECK_QUANTITY_MASK = 3;
        string err_mess;

        // Сначала, если это требуется полем check_mode определителя вызываемого метода, проверим количественное соответствие между
        // actual_args и требованиями к количеству фактическаих параметров, предъявляемых вызываемым методом втыкалы.
        int quantity_param_check_mode = method_def.check_mode & PARAM_CHECK_QUANTITY_MASK;
        if (quantity_param_check_mode && method_def.arg_count_min == method_def.arg_count_max)
        { // При этих условиях проводим проверку на точное соответствие числа затребованных и действительных фактических параметров.
            if (actual_args.size() != method_def.arg_count_min)
            { // Наличное число фактических параметров неверное (не соответствует требованиям).
                err_mess = ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_METHOD) + method_def.name +
                    ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_DEMAND_EQUAL) +
                    to_string(method_def.arg_count_min) + ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_ARGUMENTS);
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT, err_mess);
            }
            else
            { // Действительное количество фактических параметров правильное.
                return;
            }
        }
        // При неравных величинах method_def.arg_count_min и method_def.arg_count_max применяем другие способы количественной проверки.
        if (quantity_param_check_mode && actual_args.size() > method_def.arg_count_max)
        {
            err_mess = ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_METHOD) + method_def.name +
                ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_DEMAND_LESS_OR_EQUAL) +
                to_string(method_def.arg_count_max) + ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_ARGUMENTS);
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT, err_mess);
        }
        if (quantity_param_check_mode && actual_args.size() < method_def.arg_count_min)
        {
            err_mess = ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_METHOD) + method_def.name +
                ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_DEMAND_GREATER_OR_EQUAL) +
                to_string(method_def.arg_count_min) + ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_ARGUMENTS);
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT, err_mess);
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
                    err_mess = ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_PARAMETER) + to_string(i) +
                        ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_OF_METHOD) + method_def.name +
                        ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_HAVE_INCOMPATIBLE_TYPE);
                    ThrowRuntimeError(context, ThrowMessageNumber::THRM_PARAMS_TYPE_INCONSISTENCY, err_mess);
                }
                ++i;
            }
        }
    }

    PluginInstance::PluginInstance(const std::string& class_name, const ast::PluginMethodList& methods_def_list) :
        class_name_(class_name), plugin_methods_list_(methods_def_list)
    {}

    void PluginInstance::Print(std::ostream& os, Context& context)
    {
        os << class_name_ << " - " << (void*)this;
    }

    ObjectHolder PluginInstance::Call(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                      Context& context, const std::string& parent_name)
    {
        const ast::MethodDefiner* call_method_def = GetMethod(plugin_methods_list_, method, actual_args.size());
        if (call_method_def)
        {
            CheckActualParamLegality(context, *call_method_def, actual_args);

        }
        else
        {
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_METHOD_NOT_FOUND);
        }
    }
    
    bool PluginInstance::HasMethod(const std::string& method_name, size_t argument_count) const
    {
        return GetMethod(plugin_methods_list_, method_name, argument_count);
    }
} // namespace runtime

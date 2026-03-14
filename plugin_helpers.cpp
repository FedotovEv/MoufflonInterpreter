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

extern "C"
{
    MYTHLON_KERNEL_EXPORT [[noreturn]] void ThrowRuntimeError(intptr_t context, int msg_num, const char* except_text)
    {

    }
}

#include <optional>
#include <fstream>
#include <cstdio>

using namespace std;
using namespace runtime;
using namespace std;

namespace ast
{
    NewPluginInstance::NewPluginInstance
        (std::string&& class_name, std::vector<std::unique_ptr<Statement>>&& args, PluginMethodList&& methods_def_list) :
        class_name_(move(class_name)), actual_args_(move(args)), plugin_methods_list_(move(methods_def_list))
    {
        auto ctor_argcount_it = std::find_if(plugin_methods_list_.begin(), plugin_methods_list_.end(),
            [](const MethodDefiner& method_def) -> bool
            {
                return method_def.name == "ctor";
            });

        if (ctor_argcount_it != plugin_methods_list_.end())
        {
            if (actual_args_.size() < ctor_argcount_it->arg_count_min)
                runtime::ThrowRuntimeError(this, ThrowMessageNumber::THRM_DEMAND_GREATER_OR_EQUAL, "Слишком мало аргументов для конструктора"s);
            else if (actual_args_.size() > ctor_argcount_it->arg_count_max)
                runtime::ThrowRuntimeError(this, ThrowMessageNumber::THRM_DEMAND_LESS_OR_EQUAL, "Слишком много аргументов для конструктора"s);
        }
    }

    runtime::ObjectHolder NewPluginInstance::Execute(runtime::Closure& closure, runtime::Context& context)
    {
        PrepareExecute(this, closure, context);

        auto ctor_argcount_it = std::find_if(plugin_methods_list_.begin(), plugin_methods_list_.end(),
            [](const MethodDefiner& method_def) -> bool
            {
                return method_def.name == "ctor";
            });

        if (ctor_argcount_it != plugin_methods_list_.end())
        { // Метод конструктора имеет особое описание в списке методов класса, следовательно, соответствующий ему метод существует и в теле
          // самой втыкалы. Вызовем его сейчас, передав ему в качестве фактических параметров текущие значения actual_args_.

        }
        else
        { // Конструктора сам модуль втыкалы не предусматривает.
            return ObjectHolder::Own(runtime::PluginInstance());
        }
    }
} // namespace ast

namespace runtime
{
    void CheckActualParamLegality(Context& context, const ast::MethodDefiner& method_def, const std::vector<ObjectHolder>& actual_args)
    {
        static constexpr int PARAM_CHECK_QUANTITY_MASK = 3;
        string err_mess;

        switch (method_def.check_mode & PARAM_CHECK_QUANTITY_MASK)
        {
        case MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL:
            if (actual_args.size() != required_params)
            {
                err_mess = ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_METHOD) + method_def.name +
                    ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_DEMAND_EQUAL) +
                    to_string(required_params) + ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_ARGUMENTS);
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT, err_mess);
            }
            break;
        case MethodParamCheckMode::PARAM_CHECK_QUANTITY_LESS_EQ:
            if (actual_args.size() > required_params)
            {
                err_mess = ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_METHOD) + method_def.name +
                    ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_DEMAND_LESS_OR_EQUAL) +
                    to_string(required_params) + ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_ARGUMENTS);
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT, err_mess);
            }
            break;
        case MethodParamCheckMode::PARAM_CHECK_QUANTITY_GREATER_EQ:
            if (actual_args.size() < required_params)
            {
                err_mess = ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_METHOD) + method_def.name +
                    ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_DEMAND_GREATER_OR_EQUAL) +
                    to_string(required_params) + ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_ARGUMENTS);
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT, err_mess);
            }
            break;
        default:
            break;
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
        if (array_method_table_.count(method_name))
            return (this->*array_method_table_.at(method_name))(method_name, actual_args, context);
        else
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_METHOD_NOT_FOUND);
    }
    
    bool PluginInstance::HasMethod(const std::string& method_name, size_t argument_count) const
    {
        if (map_method_argument_count_.count(method_name))
        {
            auto argument_org_count = map_method_argument_count_.at(method_name);
            return argument_count >= argument_org_count.first &&
                argument_count <= argument_org_count.second;
        }
        else
        {
            return false;
        }
    }
} // namespace runtime

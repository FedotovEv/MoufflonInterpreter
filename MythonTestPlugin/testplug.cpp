
// Двоичное тестовое дополнение - "втыкало" - для интерпретатора МУФЛОН.  Служит для работы модульных тестов интерпретатора,
// проверяющих работоспособность механизма втыкал.

#include "pch.h"

#include "testplug.h"

#include <optional>
#include <fstream>
#include <cstdio>

#define ZERO_TOLERANCE 0.000001

using namespace std;
using namespace runtime;
using namespace std;

namespace ast
{
    NewPlugin::NewPlugin(std::vector<std::unique_ptr<Statement>>&& args) : args_(move(args))
    {
        if (args_.size())
                ThrowRuntimeError(this, "Конструктор класса TestPlugin не должен иметь параметров"s);
    }

    runtime::ObjectHolder NewPlugin::Execute(runtime::Closure& closure, runtime::Context& context)
    {
        PrepareExecute(this, closure, context);
        return ObjectHolder::Own(runtime::PluginInstance());
    }
} // namespace ast

static PluginListType plugin_list = {{"CreatePlugin"s, "test"s}};

extern "C"
{
    unique_ptr<ast::Statement> CreatePlugin(vector<unique_ptr<ast::Statement>> args)
    {
        return make_unique<ast::NewPlugin>(ast::NewPlugin(move(args)));
    }
    
    PluginListType* LoadPluginList()
    {
        return &plugin_list;
    }
}

namespace runtime
{
    const unordered_map<string_view, PluginInstance::PluginCallMethod> PluginInstance::plugin_method_table_
    {
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

    void PluginInstance::Print(ostream& os, Context& context)
    {
        os << "TestPlugin:";
    }

    ObjectHolder PluginInstance::MethodTestAddAll(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                                  Context& context)
    {
        CheckMethodParams(context, "AddAll"s, MethodParamCheckMode::PARAM_CHECK_TYPE,
                          MethodParamType::PARAM_TYPE_NUMERIC, 0, actual_args);

        int int_result = 0;
        double double_result = 0.0;
        bool is_double_arg = false;
        for (auto& cur_arg : actual_args)
        {
            if (!is_double_arg && !cur_arg.TryAs<Number>()->IsInt())
            {
                is_double_arg = true;
                double_result = int_result;
            }

            if (is_double_arg)
                double_result += cur_arg.TryAs<Number>()->GetDoubleValue();
            else
                int_result += cur_arg.TryAs<Number>()->GetIntValue();
        }
        
        if (is_double_arg)
            return ObjectHolder::Own(Number(double_result));
        else
            return ObjectHolder::Own(Number(int_result));
    }
    
    ObjectHolder PluginInstance::MethodTestFindZero(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                                    Context& context)
    {
        CheckMethodParams(context, "FindZero"s, MethodParamCheckMode::PARAM_CHECK_TYPE,
                          MethodParamType::PARAM_TYPE_NUMERIC, 0, actual_args);

        int zero_element_index = 0;
        for (auto& cur_arg : actual_args)
        {
            if (cur_arg.TryAs<Number>()->IsInt())
            {
                if (cur_arg.TryAs<Number>()->GetIntValue() == 0)
                    return ObjectHolder::Own(Number(zero_element_index));
            }
            else
            {
                if (abs(cur_arg.TryAs<Number>()->GetDoubleValue()) < ZERO_TOLERANCE)
                    return ObjectHolder::Own(Number(zero_element_index));
            }
            ++zero_element_index;
        }
        
        return ObjectHolder::None();
    }
    
    ObjectHolder PluginInstance::MethodTestFindChar(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                                    Context& context)
    {
        CheckMethodParams(context, "FindChar"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_STRING, 2, actual_args);

        string match_string = actual_args[1].TryAs<String>()->GetValue();
        char match_char;
        if (match_string.empty())
            return ObjectHolder::Own(Number(0));
        else
            match_char = match_string[0];

        int char_position = actual_args[0].TryAs<String>()->GetValue().find(match_char);
        return ObjectHolder::Own(Number(char_position));
    }
    
    ObjectHolder PluginInstance::MethodTestSton(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                                Context& context)
    {
        CheckMethodParams(context, "Ston"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_STRING, 1, actual_args);

        string string_arg = actual_args[0].TryAs<String>()->GetValue();
        int int_result = stoi(string_arg);
        double double_result = stod(string_arg);
        if (static_cast<double>(int_result) == double_result)
            return ObjectHolder::Own(Number(int_result));
        else
            return ObjectHolder::Own(Number(double_result));
    }
    
    ObjectHolder PluginInstance::MethodTestPrintHello(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                                      Context& context)
    {
        CheckMethodParams(context, "PrintHello"s, MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_ANY, 0, actual_args);

        string hello_string = "Hello"s;
        context.GetOutputStream() << hello_string;
        return ObjectHolder::Own(String(move(hello_string)));
    }

    ObjectHolder PluginInstance::Call(const string& method_name,
        const vector<ObjectHolder>& actual_args, Context& context)
    {
        if (plugin_method_table_.count(method_name))
            return (this->*plugin_method_table_.at(method_name))(method_name, actual_args, context);
        else
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_METHOD_NOT_FOUND);
    }
} //namespace runtime

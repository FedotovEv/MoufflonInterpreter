
#include "statement.h"
#include "parse.h"
#include "throw_messages.h"
#include "error_classes.h"

#include <cassert>
#include <optional>
#include <sstream>
#include <cmath>

using namespace std;
using namespace runtime;

namespace ast
{
    NewMath::NewMath(std::vector<std::unique_ptr<Statement>> args)
    {
        if (args.size())
            throw ParseError(ThrowMessageNumber::THRM_MATH_CTOR_HAS_NO_PARAMS);
    }

    runtime::ObjectHolder NewMath::Execute(runtime::Closure& closure, runtime::Context& context)
    {
        PrepareExecute(this, closure, context);
        return ObjectHolder::Own(runtime::MathInstance());
    }

    NewStringOps::NewStringOps(std::vector<std::unique_ptr<Statement>> args)
    {
        if (args.size())
            throw ParseError(ThrowMessageNumber::THRM_STRINGOPS_CTOR_HAS_NO_PARAMS);
    }

    runtime::ObjectHolder NewStringOps::Execute(runtime::Closure& closure, runtime::Context& context)
    {
        PrepareExecute(this, closure, context);
        return ObjectHolder::Own(runtime::StringOpsInstance());
    }

    unique_ptr<Statement> CreateMath(vector<unique_ptr<Statement>> args)
    {
        return make_unique<NewMath>(NewMath(move(args)));
    }

    unique_ptr<Statement> CreateStringOps(vector<std::unique_ptr<Statement>> args)
    {
        return make_unique<NewStringOps>(NewStringOps(move(args)));
    }
} // namespace ast

namespace runtime
{
    const unordered_map<string_view, MathInstance::MathCallMethod> MathInstance::math_method_table_
    {
        {"abs"sv, &MathInstance::MethodAbs},
        {"Abs"sv, &MathInstance::MethodAbs},
        {"pow"sv, &MathInstance::MethodPow},
        {"Pow"sv, &MathInstance::MethodPow},
        {"sqrt"sv, &MathInstance::MethodSqrt},
        {"Sqrt"sv, &MathInstance::MethodSqrt},
        {"sin"sv, &MathInstance::MethodSin},
        {"Sin"sv, &MathInstance::MethodSin},
        {"cos"sv, &MathInstance::MethodCos},
        {"Cos"sv, &MathInstance::MethodCos},
        {"atan"sv, &MathInstance::MethodAtan},
        {"Atan"sv, &MathInstance::MethodAtan},
        {"atan2"sv, &MathInstance::MethodAtan2},
        {"Atan2"sv, &MathInstance::MethodAtan2},
        {"log"sv, &MathInstance::MethodLog},
        {"Log"sv, &MathInstance::MethodLog},
        {"exp"sv, &MathInstance::MethodExp},
        {"Exp"sv, &MathInstance::MethodExp},
        {"floor"sv, &MathInstance::MethodFloor},
        {"Floor"sv, &MathInstance::MethodFloor},
        {"ceil"sv, &MathInstance::MethodCeil},
        {"Ceil"sv, &MathInstance::MethodCeil},
        {"round"sv, &MathInstance::MethodRound},
        {"Round"sv, &MathInstance::MethodRound}
    };

    const unordered_map<string_view, pair<size_t, size_t>> MathInstance::math_method_argument_count_
    {
        {"abs"sv, {1, 1}},
        {"Abs"sv, {1, 1}},
        {"pow"sv, {2, 2}},
        {"Pow"sv, {2, 2}},
        {"sqrt"sv, {1, 1}},
        {"Sqrt"sv, {1, 1}},
        {"sin"sv, {1, 1}},
        {"Sin"sv, {1, 1}},
        {"cos"sv, {1, 1}},
        {"Cos"sv, {1, 1}},
        {"atan"sv, {1, 1}},
        {"Atan"sv, {1, 1}},
        {"atan2"sv, {2, 2}},
        {"Atan2"sv, {2, 2}},
        {"log"sv, {1, 1}},
        {"Log"sv, {1, 1}},
        {"exp"sv, {1, 1}},
        {"Exp"sv, {1, 1}},
        {"floor"sv, {1, 1}},
        {"Floor"sv, {1, 1}},
        {"ceil"sv, {1, 1}},
        {"Ceil"sv, {1, 1}},
        {"round"sv, {1, 1}},
        {"Round"sv, {1, 1}}
    };

    void MathInstance::Print(ostream& os, Context& context)
    {
        os << "Math:";
    }

    ObjectHolder MathInstance::MethodAbs(const string& method, const vector<ObjectHolder>& actual_args,
                                         Context& context)
    {
        CheckMethodParams(context, "Abs"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_NUMERIC, 1, actual_args);

        Number* number_ptr = actual_args[0].TryAs<Number>();
        if (number_ptr->IsInt())
        {
            int result = abs(number_ptr->GetIntValue());
            return ObjectHolder::Own(Number(result));
        }
        else if (number_ptr->IsDouble())
        {
            double result = abs(number_ptr->GetDoubleValue());
            return ObjectHolder::Own(Number(result));
        }
        else
        {
            return ObjectHolder::Own(Number(0));
        }
    }

    ObjectHolder MathInstance::MethodPow(const string& method, const vector<ObjectHolder>& actual_args,
                                         Context& context)
    {
        CheckMethodParams(context, "Pow"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_NUMERIC, 2, actual_args);

        Number* base_arg_ptr = actual_args[0].TryAs<Number>();
        Number* exp_arg_ptr = actual_args[1].TryAs<Number>();
        int exp_int_value = exp_arg_ptr->GetIntValue();

        if (base_arg_ptr->IsInt() && exp_arg_ptr->IsInt() && exp_int_value > 0)
        {
            int result = 1, base_value = base_arg_ptr->GetIntValue();
            for (int i = 0; i < exp_int_value; ++i)
                result *= base_value;
            return ObjectHolder::Own(Number(result));
        }
        else
        {
            return ObjectHolder::Own(Number(pow(base_arg_ptr->GetDoubleValue(), exp_arg_ptr->GetDoubleValue())));
        }
    }

    ObjectHolder MathInstance::MethodSqrt(const string& method, const vector<ObjectHolder>& actual_args,
                                          Context& context)
    {
        CheckMethodParams(context, "Sqrt"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_NUMERIC, 1, actual_args);

        return ObjectHolder::Own(Number(sqrt(actual_args[0].TryAs<Number>()->GetDoubleValue())));
    }

    ObjectHolder MathInstance::MethodSin(const string& method, const vector<ObjectHolder>& actual_args,
                                         Context& context)
    {
        CheckMethodParams(context, "Sin"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_NUMERIC, 1, actual_args);

        return ObjectHolder::Own(Number(sin(actual_args[0].TryAs<Number>()->GetDoubleValue())));
    }

    ObjectHolder MathInstance::MethodCos(const string& method, const vector<ObjectHolder>& actual_args,
                                         Context& context)
    {
        CheckMethodParams(context, "Cos"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_NUMERIC, 1, actual_args);

        return ObjectHolder::Own(Number(cos(actual_args[0].TryAs<Number>()->GetDoubleValue())));
    }

    ObjectHolder MathInstance::MethodAtan(const string& method, const vector<ObjectHolder>& actual_args,
                                          Context& context)
    {
        CheckMethodParams(context, "Atan"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_NUMERIC, 1, actual_args);

        return ObjectHolder::Own(Number(atan(actual_args[0].TryAs<Number>()->GetDoubleValue())));
    }

    ObjectHolder MathInstance::MethodAtan2(const string& method, const vector<ObjectHolder>& actual_args,
                                           Context& context)
    {
        CheckMethodParams(context, "Atan2"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_NUMERIC, 2, actual_args);

        double y_arg = actual_args[0].TryAs<Number>()->GetDoubleValue();
        double x_arg = actual_args[1].TryAs<Number>()->GetDoubleValue();

        return ObjectHolder::Own(Number(atan2(y_arg, x_arg)));
    }

    ObjectHolder MathInstance::MethodLog(const string& method, const vector<ObjectHolder>& actual_args,
                                         Context& context)
    {
        CheckMethodParams(context, "Log"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_NUMERIC, 1, actual_args);

        return ObjectHolder::Own(Number(log(actual_args[0].TryAs<Number>()->GetDoubleValue())));
    }

    ObjectHolder MathInstance::MethodExp(const string& method, const vector<ObjectHolder>& actual_args,
                                         Context& context)
    {
        CheckMethodParams(context, "Exp"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_NUMERIC, 1, actual_args);

        return ObjectHolder::Own(Number(exp(actual_args[0].TryAs<Number>()->GetDoubleValue())));
    }

    ObjectHolder MathInstance::MethodCeil(const string& method, const vector<ObjectHolder>& actual_args,
                                          Context& context)
    {
        CheckMethodParams(context, "Ceil"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_NUMERIC, 1, actual_args);

        return ObjectHolder::Own(Number(static_cast<int>(ceil(actual_args[0].TryAs<Number>()->GetDoubleValue()))));
    }

    ObjectHolder MathInstance::MethodFloor(const string& method, const vector<ObjectHolder>& actual_args,
                                           Context& context)
    {
        CheckMethodParams(context, "Floor"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_NUMERIC, 1, actual_args);

        return ObjectHolder::Own(Number(static_cast<int>(floor(actual_args[0].TryAs<Number>()->GetDoubleValue()))));
    }

    ObjectHolder MathInstance::MethodRound(const string& method, const vector<ObjectHolder>& actual_args,
                                           Context& context)
    {
        CheckMethodParams(context, "Round"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_NUMERIC, 1, actual_args);

        return ObjectHolder::Own(Number(static_cast<int>(round(actual_args[0].TryAs<Number>()->GetDoubleValue()))));
    }

    ObjectHolder MathInstance::Call(const string& method_name, const vector<ObjectHolder>& actual_args,
                                    Context& context, const std::string& parent_name)
    {
        if (math_method_table_.count(method_name))
            return (this->*math_method_table_.at(method_name))(method_name, actual_args, context);
        else
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_METHOD_NOT_FOUND);
    }

    bool MathInstance::HasMethod(const string& method_name, size_t argument_count) const
    {
        if (math_method_argument_count_.count(method_name))
        {
            auto argument_org_count = math_method_argument_count_.at(method_name);
            return argument_count >= argument_org_count.first &&
                   argument_count <= argument_org_count.second;
        }
        else
        {
            return false;
        }
    }

    // Таблицы описания методов класса StringOpsInstance и конвенций их вызова.
    const std::unordered_map<std::string_view, StringOpsInstance::StringOpsCallMethod> StringOpsInstance::string_ops_method_table_
    {

    };
    
    const std::unordered_map<std::string_view, std::pair<size_t, size_t>> StringOpsInstance::string_ops_method_argument_count_
    {

    };

    // Вспомогательная функция-член извлечения пары фактических параметров подстроки - начального её индекса и длины.
    std::pair<size_t, size_t> StringOpsInstance::ExtractPosSize
        (const std::vector<ObjectHolder>& actual_args, size_t arg_start_pos, const std::string& arg_str, Context& context)
    {
        // Значения извлекаемых параметров подстроки по умолчанию.
        size_t arg_pos = 0;
        size_t arg_count = std::string::npos;

        if (actual_args.size() >= 2)
        { // Явно задан arg_pos.
            const runtime::Number* arg_pos_ptr = actual_args[arg_start_pos].TryAs<runtime::Number>();
            if (!arg_pos_ptr)
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_TYPE, "Позиция в строке должна быть числом");
            int arg_pos_int = arg_pos_ptr->GetIntValue();
            if (arg_pos_int < 0 || arg_pos_int > static_cast<int>(arg_str.size()))
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_VALUE, "Указанная позиция в строке недопустима");
            arg_pos = static_cast<size_t>(arg_pos_int);
        }

        if (actual_args.size() >= 3)
        { // Явно указан arg_count.
            const runtime::Number* arg_count_ptr = actual_args[arg_start_pos + 1].TryAs<runtime::Number>();
            if (!arg_count_ptr)
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_TYPE, "Длина подстроки должна быть числом");
            int arg_count_int = arg_count_ptr->GetIntValue();
            if (arg_count_int < 0)
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_VALUE, "Длина подстроки должна быть неотрицательной");
            arg_count = static_cast<size_t>(arg_count_int);
        }

        return {arg_pos, arg_count};
    }

    // Обобщённый поиск подстроки в строке, который для каждого конкретной разновидности отличается только передаваемой поисковой функцией find_func.
    ObjectHolder StringOpsInstance::MethodCommonFind
        (const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context, size_t default_pos, CommonFindFunc find_func)
    {
        MethodParamCheckMode param_check_mode = static_cast<MethodParamCheckMode>
            (MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_GREATER_EQ & MethodParamCheckMode::PARAM_CHECK_TYPE_ONLY_FOR_MIN_ARGS);
        CheckMethodParams(context, method, param_check_mode, MethodParamType::PARAM_TYPE_STRING, 2, actual_args);
        if (actual_args.size() > 3) // Допускается от 2 до 3 параметров (включительно).
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT, "Метод " + method + " может принимать 2 или 3 параметра");

        std::string arg_str = actual_args[0].TryAs<runtime::String>()->GetValue(),
                    arg_str_what = actual_args[1].TryAs<runtime::String>()->GetValue();
        size_t arg_pos = default_pos; // Значение начальной (или конечной) позиции поиска по умолчанию.

        if (actual_args.size() >= 3)
        { // Явно задан arg_pos.
            const runtime::Number* arg_pos_ptr = actual_args[2].TryAs<runtime::Number>();
            if (!arg_pos_ptr)
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_TYPE, "Позиция в строке должна быть числом");
            int arg_pos_int = arg_pos_ptr->GetIntValue();
            if (arg_pos_int < 0)
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_VALUE, "Позиция в строке должна быть неотрицательной");
            arg_pos = static_cast<size_t>(arg_pos_int);
        }

        // Все параметры предстоящей операции определены и проверены. Можно выполнять.
        return ObjectHolder::Own(runtime::Number(static_cast<int>((arg_str.*find_func)(arg_str_what, arg_pos))));
    }

    ObjectHolder StringOpsInstance::MethodSize(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        CheckMethodParams(context, "Size"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_STRING, 1, actual_args);

        return ObjectHolder::Own(runtime::Number(static_cast<int>(actual_args[0].TryAs<runtime::String>()->GetValue().size())));
    }
    
    ObjectHolder StringOpsInstance::MethodConcat(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        CheckMethodParams(context, "Concat"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_GREATER_EQ,
                          MethodParamType::PARAM_TYPE_STRING, 1, actual_args);

        std::string result_string;
        for (const ObjectHolder& arg_string_holder : actual_args)
            result_string += arg_string_holder.TryAs<runtime::String>()->GetValue();

        return ObjectHolder::Own(runtime::String(move(result_string)));
    }
    
    // append(arg_str_to, arg_str_what, arg_pos, arg_count) - присоединение к строке arg_str_to arg_count символов строки arg_str_what,
    // начиная с позиции arg_pos в ней.
    ObjectHolder StringOpsInstance::MethodAppend(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        MethodParamCheckMode param_check_mode = static_cast<MethodParamCheckMode>
            (MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_GREATER_EQ & MethodParamCheckMode::PARAM_CHECK_TYPE_ONLY_FOR_MIN_ARGS);
        CheckMethodParams(context, "Append"s, param_check_mode, MethodParamType::PARAM_TYPE_STRING, 2, actual_args);
        if (actual_args.size() > 4) // Допускается от 2 до 4 параметров (включительно).
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT, "Метод append может принимать от 2 до 4 параметров");

        std::string arg_str_to = actual_args[0].TryAs<runtime::String>()->GetValue(),
                    arg_str_what = actual_args[1].TryAs<runtime::String>()->GetValue();
        auto [arg_pos, arg_count] = ExtractPosSize(actual_args, 2, arg_str_what, context);

        // Все параметры предстоящей операции определены и проверены. Можно выполнять.
        return ObjectHolder::Own(runtime::String(arg_str_to.append(arg_str_what, arg_pos, arg_count)));
    }

    // substr(arg_str, arg_pos, arg_length) - извлечение подстроки из строки arg_str длиной не более arg_length символов,
    // начиная с символа с индексом arg_pos.
    ObjectHolder StringOpsInstance::MethodSubstr(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        MethodParamCheckMode param_check_mode = static_cast<MethodParamCheckMode>
            (MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_GREATER_EQ & MethodParamCheckMode::PARAM_CHECK_TYPE_ONLY_FOR_MIN_ARGS);
        CheckMethodParams(context, "Substr"s, param_check_mode, MethodParamType::PARAM_TYPE_STRING, 1, actual_args);

        std::string arg_str = actual_args[0].TryAs<runtime::String>()->GetValue();
        auto [arg_pos, arg_count] = ExtractPosSize(actual_args, 1, arg_str, context);

        // Все параметры предстоящей операции определены и проверены. Можно выполнять.
        return ObjectHolder::Own(runtime::String(arg_str.substr(arg_pos, arg_count)));
    }

    // find(arg_str_haystack, arg_str_needle, arg_pos) - поиск первого вхождения подстроки arg_str_needle в строку arg_str_haystack,
    // начиная с позиции arg_pos.
    ObjectHolder StringOpsInstance::MethodFind(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        return MethodCommonFind(method, actual_args, context, 0, &std::string::find);
    }
    
    // rfind(arg_str_haystack, arg_str_needle, arg_pos) - поиск последнего вхождения подстроки arg_str_needle в строку arg_str_haystack,
    // начиная с позиции arg_pos.
    ObjectHolder StringOpsInstance::MethodRfind(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        return MethodCommonFind(method, actual_args, context, std::string::npos, &std::string::rfind);
    }
    
    //find_first_of(arg_str_haystack, arg_str_needle_list, arg_pos) - поиск первого вхождения любого символа строки arg_str_needle в
    // строку arg_str_haystack, начиная с позиции arg_pos.
    ObjectHolder StringOpsInstance::MethodFindFirstOf
        (const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        return MethodCommonFind(method, actual_args, context, 0, &std::string::find_first_of);
    }
    
    ObjectHolder StringOpsInstance::MethodFindFirstNotOf
        (const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        return MethodCommonFind(method, actual_args, context, 0, &std::string::find_first_not_of);
    }
    
    ObjectHolder StringOpsInstance::MethodFindLastOf
        (const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        return MethodCommonFind(method, actual_args, context, std::string::npos, &std::string::find_last_of);
    }
    
    ObjectHolder StringOpsInstance::MethodFindLastNotOf
        (const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        return MethodCommonFind(method, actual_args, context, std::string::npos, &std::string::find_last_not_of);
    }
    
    ObjectHolder StringOpsInstance::MethodStartsWith(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        CheckMethodParams(context, "StartsWith"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_STRING, 2, actual_args);

        std::string arg_str_haystack = actual_args[0].TryAs<runtime::String>()->GetValue(),
                    arg_str_needle = actual_args[1].TryAs<runtime::String>()->GetValue();
        return ObjectHolder::Own(runtime::Bool(arg_str_haystack.starts_with(arg_str_needle)));
    }
    
    ObjectHolder StringOpsInstance::MethodEndsWith(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        CheckMethodParams(context, "EndsWith"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_STRING, 2, actual_args);

        std::string arg_str_haystack = actual_args[0].TryAs<runtime::String>()->GetValue(),
                    arg_str_needle = actual_args[1].TryAs<runtime::String>()->GetValue();
        return ObjectHolder::Own(runtime::Bool(arg_str_haystack.ends_with(arg_str_needle)));
    }
    
    ObjectHolder StringOpsInstance::MethodContains(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        CheckMethodParams(context, "Contains"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_STRING, 2, actual_args);

        std::string arg_str_haystack = actual_args[0].TryAs<runtime::String>()->GetValue(),
                    arg_str_needle = actual_args[1].TryAs<runtime::String>()->GetValue();
        return ObjectHolder::Own(runtime::Bool(arg_str_haystack.find(arg_str_needle) != std::string::npos));
    }
    
    // insert(arg_str, arg_pos, arg_str_ins, arg_count) - вставка строки arg_str_ins в количестве arg_count экземпляров в строку arg_str
    // в положение arg_pos.
    ObjectHolder StringOpsInstance::MethodInsert(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        CheckMethodParams(context, "Insert"s, MethodParamCheckMode::PARAM_CHECK_QUANTITY_GREATER_EQ,
                          MethodParamType::PARAM_TYPE_ANY, 3, actual_args);
        if (actual_args.size() > 4) // Допускается от 3 до 4 параметров (включительно). Только arg_count может быть опущен.
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT, "Метод Insert может принимать от 3 до 4 параметров");

        runtime::String* arg_str_ptr = actual_args[0].TryAs<runtime::String>();
        runtime::Number* arg_pos_ptr = actual_args[1].TryAs<runtime::Number>();
        runtime::String* arg_str_ins_ptr = actual_args[2].TryAs<runtime::String>();

        if (!arg_str_ptr || !arg_str_ins_ptr)
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_TYPE, "Для метод Insert не заданы исходная или вставляемая строка");
        if (!arg_pos_ptr)
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_TYPE, "Для метод Insert не задан индекс точки вставки");

        std::string arg_str = arg_str_ptr->GetValue();
        std::string arg_str_ins = arg_str_ins_ptr->GetValue();
        size_t arg_pos = arg_pos_ptr->GetIntValue();
        if (arg_pos < 0 || arg_pos > arg_str.size())
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_VALUE, "Insert : указанная позиция в строке недопустима");

        size_t arg_count = 1; // По умолчанию вставляется один экземпляр arg_str_ins.
        if (actual_args.size() >= 3)
        { // Явно задан arg_count.
            const runtime::Number* arg_count_ptr = actual_args[1].TryAs<runtime::Number>();
            if (!arg_count_ptr)
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_TYPE, "Количество экземпляров должно быть числом");
            int arg_count_int = arg_count_ptr->GetIntValue();
            if (arg_count_int < 0)
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_VALUE, "Количество копий должно быть неотрицательным");
            arg_count = static_cast<size_t>(arg_count_int);
        }

        std::string summ_insert_str;
        for (size_t i = 1; i <= arg_count; ++i)
            summ_insert_str += arg_str_ins;

        return ObjectHolder::Own(runtime::String(arg_str.insert(arg_pos, summ_insert_str)));
    }
    
    // erase(arg_str, arg_pos, arg_length) - удаление arg_length символов из строки arg_str, начиная с положения arg_pos в ней.
    ObjectHolder StringOpsInstance::MethodErase(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        MethodParamCheckMode param_check_mode = static_cast<MethodParamCheckMode>
            (MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_GREATER_EQ & MethodParamCheckMode::PARAM_CHECK_TYPE_ONLY_FOR_MIN_ARGS);
        CheckMethodParams(context, "Erase"s, param_check_mode, MethodParamType::PARAM_TYPE_STRING, 1, actual_args);
        if (actual_args.size() > 3) // Допускается от 1 до 3 параметров (включительно).
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT, "Метод Erase может принимать от 1 до 3 параметров");

        std::string arg_str = actual_args[0].TryAs<runtime::String>()->GetValue();
        auto [arg_pos, arg_length] = ExtractPosSize(actual_args, 1, arg_str, context);
        return ObjectHolder::Own(runtime::String(arg_str.erase(arg_pos, arg_length)));
    }
    
    // replace(arg_str, arg_pos, arg_count, arg_str_ins, arg_pos_ins, arg_count_ins) - замена arg_count символов строки arg_str, начиная с
    // положения arg_pos, на arg_count_ins символов строки arg_str_ins, взятых с позиции arg_pos_ins в ней.
    ObjectHolder StringOpsInstance::MethodReplace(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        CheckMethodParams(context, "Replace"s, MethodParamCheckMode::PARAM_CHECK_QUANTITY_GREATER_EQ,
                          MethodParamType::PARAM_TYPE_ANY, 4, actual_args);
        if (actual_args.size() > 6)
            // Допускается от 4 до 6 аргументов. arg_pos_ins и arg_count_ins могут не указаны явно и быть приняты по умолчанию.
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT, "Метод Replace может принимать от 4 до 6 параметров");

        runtime::String* arg_str_ptr = actual_args[0].TryAs<runtime::String>();
        runtime::String* arg_str_ins_ptr = actual_args[3].TryAs<runtime::String>();
        if (!arg_str_ptr || !arg_str_ins_ptr)
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_TYPE, "Для метод Replace не заданы исходная или заменяющая строка");

        std::string arg_str = arg_str_ptr->GetValue();
        std::string arg_str_ins = arg_str_ins_ptr->GetValue();
        auto [arg_pos, arg_count] = ExtractPosSize(actual_args, 1, arg_str, context);
        auto [arg_pos_ins, arg_count_ins] = ExtractPosSize(actual_args, 4, arg_str_ins, context);

        return ObjectHolder::Own(runtime::String(arg_str.replace(arg_pos, arg_count, arg_str_ins, arg_pos_ins, arg_count_ins)));
    }
    
    // replicate(arg_str, arg_count) - конструирование строки из arg_count копий строки arg_str.
    ObjectHolder StringOpsInstance::MethodReplicate(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        MethodParamCheckMode param_check_mode = static_cast<MethodParamCheckMode>
            (MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_GREATER_EQ & MethodParamCheckMode::PARAM_CHECK_TYPE_ONLY_FOR_MIN_ARGS);
        CheckMethodParams(context, "Replicate"s, param_check_mode, MethodParamType::PARAM_TYPE_STRING, 1, actual_args);
        if (actual_args.size() > 2) // Допускается 1 или 2 параметра.
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT, "Метод Replicate может принимать 1 или 2 параметра");

        std::string arg_str = actual_args[0].TryAs<runtime::String>()->GetValue();
        int arg_count = 1; // По умолчанию создаём одну копию аргумента.
        if (actual_args.size() >= 3)
        { // Явно указан arg_count.
            const runtime::Number* arg_count_ptr = actual_args[2].TryAs<runtime::Number>();
            if (!arg_count_ptr)
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_TYPE, "Количество копий строки должно быть числом");
            arg_count = arg_count_ptr->GetIntValue();
            if (arg_count < 0)
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_VALUE, "Число копий должно быть неотрицательным");
        }

        std::string result;
        for (int i = 1; i <= arg_count; ++i)
            result += arg_str;

        return ObjectHolder::Own(runtime::String(move(result)));
    }
    
    // to_number(arg_str, arg_pos, base_value) - преобразование в числовую форму фрагмента строки arg_str, начинающегося с arg_pos,
    // представляющего некоторое число в base_value - ичной системе счисления.
    ObjectHolder StringOpsInstance::MethodToNumber(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        MethodParamCheckMode param_check_mode = static_cast<MethodParamCheckMode>
            (MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_GREATER_EQ & MethodParamCheckMode::PARAM_CHECK_TYPE_ONLY_FOR_MIN_ARGS);
        CheckMethodParams(context, "ToNumber"s, param_check_mode, MethodParamType::PARAM_TYPE_STRING, 1, actual_args);
        if (actual_args.size() > 3) // Допускается от 1 до 3 параметров (включительно).
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT, "Метод ToNumber может принимать от 1 до 3 параметров");

        std::string arg_str = actual_args[0].TryAs<runtime::String>()->GetValue();
        // Значения фактических аргументов по умолчанию.
        size_t arg_pos = 0;
        int arg_radix = 0;

        if (actual_args.size() >= 2)
        { // Явно задан arg_pos.
            const runtime::Number* arg_pos_ptr = actual_args[1].TryAs<runtime::Number>();
            if (!arg_pos_ptr)
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_TYPE, "Позиция в строке должна быть числом");
            int arg_pos_int = arg_pos_ptr->GetIntValue();
            if (arg_pos_int < 0 || arg_pos_int > static_cast<int>(arg_str.size()))
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_VALUE, "Указанная позиция в строке недопустима");
            arg_pos = static_cast<size_t>(arg_pos_int);
        }

        if (actual_args.size() >= 3)
        { // Явно указан arg_radix.
            const runtime::Number* arg_radix_ptr = actual_args[2].TryAs<runtime::Number>();
            if (!arg_radix_ptr)
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_TYPE, "База преобразуемого числа также должна быть числом");
            arg_radix = arg_radix_ptr->GetIntValue();
            if ((arg_radix != 0) && (arg_radix < 2 || arg_radix > 36))
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_VALUE, "Задана недопустимая база преобразуемого числа");
        }

        const char* begin_number_pos = arg_str.c_str() + arg_pos;
        char* end_number_pos;

        // Попробуем преобразовать строку в целое число.
        errno = 0;
        long long int_result = strtoll(begin_number_pos, &end_number_pos, arg_radix);
        int int_error = errno;
        size_t int_result_length = end_number_pos - begin_number_pos;

        // А теперь - в дробное.
        errno = 0;
        double double_result = strtod(begin_number_pos, &end_number_pos);
        int double_error = errno;
        size_t double_result_length = end_number_pos - begin_number_pos;

        // Выберем наиболее подходящим тот результат, который использовал больше символов исходника.
        if (double_result_length > int_result_length)
        { // Используем результат преобразования в дробное число.
            last_to_number_error_ = double_error;
            last_to_number_length_ = static_cast<int>(double_result_length);
            return ObjectHolder::Own(runtime::Number(double_result));
        }
        else
        { // Используем результат преобразования в целое число.
            if (int_result < INT_MIN || int_result > INT_MAX)
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_OVERFLOW, "ToNumber - результат вне допустимого диапазона");
            last_to_number_error_ = int_error;
            last_to_number_length_ = static_cast<int>(int_result_length);
            return ObjectHolder::Own(runtime::Number(static_cast<int>(int_result)));
        }
    }
    
    // to_number_length() - возврат длины подстроки, которую удалось преобразовать в число в ходе последнего вызова to_number().
    ObjectHolder StringOpsInstance::MethodToNumberLength
        (const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        return ObjectHolder::Own(runtime::Number(last_to_number_length_));
    }
    
    // to_number_error() - код ошибки, которая могла возникнуть при последнем вызове to_number().
    ObjectHolder StringOpsInstance::MethodToNumberError
        (const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        return ObjectHolder::Own(runtime::Number(last_to_number_error_));
    }
    
    // to_string(arg_number) - преобразование в строку числового аргумента arg_number.
    ObjectHolder StringOpsInstance::MethodToString(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        CheckMethodParams(context, "ToString"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_NUMERIC, 1, actual_args);
        
        const runtime::Number* arg_num = actual_args[2].TryAs<runtime::Number>();
        if (arg_num->IsDouble())
            return ObjectHolder::Own(runtime::String(std::to_string(arg_num->GetDoubleValue())));
        else
            return ObjectHolder::Own(runtime::String(std::to_string(arg_num->GetIntValue())));
    }

    ObjectHolder StringOpsInstance::Call
        (const std::string& method_name, const std::vector<ObjectHolder>& actual_args, Context& context, const std::string& parent_name)
    {
        if (string_ops_method_table_.contains(method_name))
            return (this->*string_ops_method_table_.at(method_name))(method_name, actual_args, context);
        else
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_METHOD_NOT_FOUND);
    }
    
    bool StringOpsInstance::HasMethod(const std::string& method_name, size_t argument_count) const
    {
        if (string_ops_method_argument_count_.contains(method_name))
        {
            auto argument_org_count = string_ops_method_argument_count_.at(method_name);
            return argument_count >= argument_org_count.first &&
                   argument_count <= argument_org_count.second;
        }
        else
        {
            return false;
        }
    }
} //namespace runtime

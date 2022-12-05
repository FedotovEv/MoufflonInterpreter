
#include "statement.h"
#include "parse.h"
#include "throw_messages.h"

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
        PrepareExecute(this, context);
        return ObjectHolder::Own(runtime::MathInstance());
    }

    unique_ptr<Statement> CreateMath(vector<unique_ptr<Statement>> args)
    {
        return make_unique<NewMath>(NewMath(move(args)));
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

    ObjectHolder MathInstance::Call(const string& method_name,
                            const vector<ObjectHolder>& actual_args, Context& context)
    {
        if (math_method_table_.count(method_name))
            return (this->*math_method_table_.at(method_name))(method_name, actual_args, context);
        else
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_METHOD_NOT_FOUND);
    }
} //namespace runtime

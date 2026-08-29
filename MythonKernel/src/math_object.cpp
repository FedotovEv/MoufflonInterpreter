
#include "statement.h"
#include "parse.h"
#include "throw_messages.h"
#include "error_classes.h"
#include "encodings.h"

#include <cassert>
#include <optional>
#include <sstream>
#include <cmath>
#include <charconv>

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

    bool MathInstance::HasMethod(const string& method_name, size_t argument_count, const std::string& parent_name) const
    {
        if (!parent_name.empty())
            return false;

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
        {"size"sv, &StringOpsInstance::MethodSize},
        {"Size"sv, &StringOpsInstance::MethodSize},
        {"length"sv, &StringOpsInstance::MethodSize},
        {"Length"sv, &StringOpsInstance::MethodSize},
        {"concat"sv, &StringOpsInstance::MethodConcat},
        {"Concat"sv, &StringOpsInstance::MethodConcat},
        {"append"sv, &StringOpsInstance::MethodAppend},
        {"Append"sv, &StringOpsInstance::MethodAppend},
        {"substr"sv, &StringOpsInstance::MethodSubstr},
        {"Substr"sv, &StringOpsInstance::MethodSubstr},
        {"find"sv, &StringOpsInstance::MethodFind},
        {"Find"sv, &StringOpsInstance::MethodFind},
        {"rfind"sv, &StringOpsInstance::MethodRFind},
        {"RFind"sv, &StringOpsInstance::MethodRFind},
        {"find_first_of"sv, &StringOpsInstance::MethodFindFirstOf},
        {"FindFirstOf"sv, &StringOpsInstance::MethodFindFirstOf},
        {"find_first_not_of"sv, &StringOpsInstance::MethodFindFirstNotOf},
        {"FindFirstNotOf"sv, &StringOpsInstance::MethodFindFirstNotOf},
        {"find_last_of"sv, &StringOpsInstance::MethodFindLastOf},
        {"FindLastOf"sv, &StringOpsInstance::MethodFindLastOf},
        {"find_last_not_of"sv, &StringOpsInstance::MethodFindLastNotOf},
        {"FindLastNotOf"sv, &StringOpsInstance::MethodFindLastNotOf},
        {"starts_with"sv, &StringOpsInstance::MethodStartsWith},
        {"StartsWith"sv, &StringOpsInstance::MethodStartsWith},
        {"ends_with"sv, &StringOpsInstance::MethodEndsWith},
        {"EndsWith"sv, &StringOpsInstance::MethodEndsWith},
        {"contains"sv, &StringOpsInstance::MethodContains},
        {"Contains"sv, &StringOpsInstance::MethodContains},
        {"not_found"sv, &StringOpsInstance::MethodNotFound},
        {"NotFound"sv, &StringOpsInstance::MethodNotFound},
        {"insert"sv, &StringOpsInstance::MethodInsert},
        {"Insert"sv, &StringOpsInstance::MethodInsert},
        {"erase"sv, &StringOpsInstance::MethodErase},
        {"Erase"sv, &StringOpsInstance::MethodErase},
        {"replace"sv, &StringOpsInstance::MethodReplace},
        {"Replace"sv, &StringOpsInstance::MethodReplace},
        {"replicate"sv, &StringOpsInstance::MethodReplicate},
        {"Replicate"sv, &StringOpsInstance::MethodReplicate},
        {"reverse"sv, &StringOpsInstance::MethodReverse},
        {"Reverse"sv, &StringOpsInstance::MethodReverse},
        {"asc"sv, &StringOpsInstance::MethodAsc},
        {"Asc"sv, &StringOpsInstance::MethodAsc},
        {"chr"sv, &StringOpsInstance::MethodChr},
        {"Chr"sv, &StringOpsInstance::MethodChr},
        {"to_number"sv, &StringOpsInstance::MethodToNumber},
        {"ToNumber"sv, &StringOpsInstance::MethodToNumber},
        {"to_number_length"sv, &StringOpsInstance::MethodToNumberLength},
        {"ToNumberLength"sv, &StringOpsInstance::MethodToNumberLength},
        {"to_number_error"sv, &StringOpsInstance::MethodToNumberError},
        {"ToNumberError"sv, &StringOpsInstance::MethodToNumberError},
        {"to_string"sv, &StringOpsInstance::MethodToString},
        {"ToString"sv, &StringOpsInstance::MethodToString}
    };
    
    const std::unordered_map<std::string_view, std::pair<size_t, size_t>> StringOpsInstance::string_ops_method_argument_count_
    {
        {"size"sv, {1, 1}},
        {"Size"sv, {1, 1}},
        {"length"sv, {1, 1}},
        {"Length"sv, {1, 1}},
        {"concat"sv, {1, (std::numeric_limits<size_t>::max)()}},
        {"Concat"sv, {1, (std::numeric_limits<size_t>::max)()}},
        {"append"sv, {2, 4}},
        {"Append"sv, {2, 4}},
        {"substr"sv, {1, 3}},
        {"Substr"sv, {1, 3}},
        {"find"sv, {2, 3}},
        {"Find"sv, {2, 3}},
        {"rfind"sv, {2, 3}},
        {"RFind"sv, {2, 3}},
        {"find_first_of"sv, {2, 3}},
        {"FindFirstOf"sv, {2, 3}},
        {"find_first_not_of"sv, {2, 3}},
        {"FindFirstNotOf"sv, {2, 3}},
        {"find_last_of"sv, {2, 3}},
        {"FindLastOf"sv, {2, 3}},
        {"find_last_not_of"sv, {2, 3}},
        {"FindLastNotOf"sv, {2, 3}},
        {"starts_with"sv, {2, 2}},
        {"StartsWith"sv, {2, 2}},
        {"ends_with"sv, {2, 2}},
        {"EndsWith"sv, {2, 2}},
        {"contains"sv, {2, 2}},
        {"Contains"sv, {2, 2}},
        {"not_found"sv, {0, 0}},
        {"NotFound"sv, {0, 0}},
        {"insert"sv, {3, 4}},
        {"Insert"sv, {3, 4}},
        {"erase"sv, {1, 3}},
        {"Erase"sv, {1, 3}},
        {"replace"sv, {4, 6}},
        {"Replace"sv, {4, 6}},
        {"replicate"sv, {1, 2}},
        {"Replicate"sv, {1, 2}},
        {"reverse"sv, {1, 1}},
        {"Reverse"sv, {1, 1}},
        {"asc"sv, {1, 2}},
        {"Asc"sv, {1, 2}},
        {"chr"sv, {1, (std::numeric_limits<size_t>::max)()}},
        {"Chr"sv, {1, (std::numeric_limits<size_t>::max)()}},
        {"to_number"sv, {1, 3}},
        {"ToNumber"sv, {1, 3}},
        {"to_number_length"sv, {0, 0}},
        {"ToNumberLength"sv, {0, 0}},
        {"to_number_error"sv, {0, 0}},
        {"ToNumberError"sv, {0, 0}},
        {"to_string"sv, {1, 3}},
        {"ToString"sv, {1, 3}}
    };

    // Вспомогательная функция-член извлечения пары фактических параметров подстроки - начального её байтового индекса и байтовой длины.
    std::pair<size_t, size_t> StringOpsInstance::ExtractPosSize
        (const std::vector<ObjectHolder>& actual_args, size_t arg_start_pos, const runtime::String* arg_str, Context& context)
    {
        // Значения извлекаемых параметров подстроки по умолчанию.
        size_t arg_pos = 0;
        const std::string& arg_str_std = arg_str->GetValue();

        if (actual_args.size() >= 2)
        { // Явно задан arg_pos.
            const runtime::Number* arg_pos_ptr = actual_args[arg_start_pos].TryAs<runtime::Number>();
            if (!arg_pos_ptr)
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_TYPE, "Позиция в строке должна быть числом");
            int arg_pos_int = arg_pos_ptr->GetIntValue();
            if (arg_pos_int < 0)
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_VALUE, "Позиция в строке должна быть неотрицательной");
            arg_pos = static_cast<size_t>(arg_pos_int);
        }

        // Для кодировки типа UTF-8 позиция arg_pos трактуется как номер (отсчитывающийся от нуля) многобайтового символа входной строки.
        // Для однобайтовых кодировок позиция arg_pos воспринимается как индекс байта в строке.
        // Если позиция в строке указывает сразу за её конец (на следующий её символ после завершающего), то такое значение позиции
        // также будем считать допустимым.
        size_t arg_byte_pos = 0;
        bool is_arg_pos_correct = true;
        if (arg_str->encoding == UTF_8_ENCODING)
        {
            if (arg_pos > arg_str->utf8_map.begin_map.size())
            {
                is_arg_pos_correct = false;
            }
            else
            { // Здесь рассматриваются допустимые варианты arg_pos - как меньше количества символов в UTF-8-строке, так и "закрайний" символ,
              // для которого arg_pos == длине строки-аргумента.
                if (arg_pos < arg_str->utf8_map.begin_map.size())
                    arg_byte_pos = arg_str->utf8_map.begin_map[arg_pos];
                else  // Тот самый "запредельный" символ за физическим концом строки.
                    arg_byte_pos = arg_str->utf8_map.BytePosAfterEnd();
            }
        }
        else
        { // Имеем дело с однобайтовой кодировкой.
            if (arg_pos > arg_str_std.size())
                is_arg_pos_correct = false;
            else
                arg_byte_pos = arg_pos;
        }
        if (!is_arg_pos_correct)
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_VALUE, "Указанная позиция в строке недопустима");

        // Обработка аргумента, указывающего длину требуемой подстроки.
        size_t arg_count = arg_str_std.size();
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
        // Рассчитаем допустимое значение длины подстроки arg_byte_count, которая может быть менее указанной, если строка arg_str слишком коротка.
        size_t arg_byte_count = 0;
        if (arg_str->encoding == UTF_8_ENCODING)
        {   // Для UTF-8-представления длина подстроки arg_count задаётся в многобайтовых Юникод-символах.
            size_t max_arg_count = arg_str->utf8_map.begin_map.size() - arg_pos;
            arg_count = min(arg_count, max_arg_count);
            // Вычислим положение конца требуемой подстроки - UTF-8-символа, следующего за концом выделяемой подстроки.
            size_t after_substr_pos = arg_pos + arg_count;   // UTF-8-позиция этого символа.
            size_t after_substr_byte_pos;                    // Байтовый индекс этого символа.
            if (after_substr_pos < arg_str->utf8_map.begin_map.size())
                after_substr_byte_pos = arg_str->utf8_map.begin_map[after_substr_pos];
            else  // Если этот символ "запредельный".
                after_substr_byte_pos = arg_str->utf8_map.BytePosAfterEnd();
            arg_byte_count = after_substr_byte_pos - arg_byte_pos;
        }
        else
        {  // Для однобайтовых кодировок длина подстроки arg_count указывается в байтах.
            size_t max_arg_count = arg_str_std.size() - arg_pos;
            arg_count = min(arg_count, max_arg_count);
            arg_byte_count = arg_count;
        }

        return {arg_byte_pos, arg_byte_count};
    }

    // Считывание идента (номера) кодировки из контейнера encoding_holder с последующей проверкой его корректности.
    int StringOpsInstance::CheckEncodingID(const ObjectHolder& encoding_holder, Context& context) const
    {
        const runtime::Number* arg_encoding = encoding_holder.TryAs<runtime::Number>(); // Индент желаемой кодировки.
        if (!arg_encoding)
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_TYPE, "Индекс кодировки должен быть численным");
        int encoding_id = arg_encoding->GetIntValue();
        if (encoding_id != NON_INDEXED_ENCODING_ID && encoding_id != NO_ENCODING_ID && encoding_id != UTF_8_ENCODING_ID)
        {
            if (encoding_id < 1 || encoding_id > static_cast<int>(::encodings_data.size()))
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_VALUE, "Индекс кодировки вне допустимого диапазона");
        }
        return encoding_id;
    }

    // Считывание имени кодировки из контейнера encoding_holder и его дальнейший поиск среди кодировок, существующих в системе.
    int StringOpsInstance::CheckEncodingName(const ObjectHolder& encoding_holder, Context& context) const
    {
        const runtime::String* arg_encoding_name = encoding_holder.TryAs<runtime::String>(); // Имя требуемой кодировки.
        if (!arg_encoding_name)
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_TYPE, "Имя кодировки должен быть строковым");

        int set_enc_id = FindEncoding(arg_encoding_name->GetValue());
        if (set_enc_id == NON_INDEXED_ENCODING_ID)
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_VALUE, "Кодировка с указанным именем не найдена");
        return set_enc_id;
    }

    // Генерация карты размещения многобайтовых UTF-8-кодов в пределах однобайтовой строки (потока байтов) parse_str.
    UTF8Map StringOpsInstance::BuildUTF8Map(const std::string& parse_str, size_t max_elem_count) const
    {
        std::pair<UTF8Map, uint32_t> build_map_result = ::BuildUTF8Map(parse_str, max_elem_count);
        const_cast<StringOpsInstance*>(this)->last_unicode_ = build_map_result.second;
        return move(build_map_result.first);
    }

    // Перекодировка МУФЛОН-строки src_string в целевую кодировку dest_encoding.
    ObjectHolder StringOpsInstance::ConvertTranscodeTo
        (const ObjectHolder& string_holder, Context& context, const SingleByteEncodingDesc* dest_encoding)
    {
        const runtime::String* src_string = string_holder.TryAs<runtime::String>();
        if (!src_string)
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_TYPE, "Аргумент должен быть строковым");

        // Выберем вид перекодировочной таблицы для исходной кодировки, в которой находится строка-аргумент.
        const std::vector<uint32_t>* src_to_utf8;       // Указатель на перекодировочную таблицу для исходной кодировки.
        if (src_string->encoding == NO_ENCODING)
            src_to_utf8 = &std_to_utf8;
        else if (src_string->encoding == UTF_8_ENCODING)
            src_to_utf8 = nullptr;
        else
            src_to_utf8 = &(src_string->encoding->to_utf8);

        // Выберем вид перекодировочной таблицы для целевой кодировки, которую требуется получить на выходе.
        const std::vector<uint32_t>* dest_to_utf8;      // Указатель на перекодировочную таблицу для целевой кодировки.
        if (dest_encoding == NO_ENCODING)
            dest_to_utf8 = &std_to_utf8;
        else if (dest_encoding == UTF_8_ENCODING)
            dest_to_utf8 = nullptr;
        else
            dest_to_utf8 = &(dest_encoding->to_utf8);

        if (src_to_utf8 == dest_to_utf8)
            // Входная и выходная кодировки эффективно одинаковы, так что просто ретранслируем входную строку на выход в неизменном виде.
            return string_holder;

        // Нужно действительное конвертирование кодировок. Выберем надлежащий тип такой конверсии.
        const std::string& src_string_std = src_string->GetValue();
        TranscodeResult conv_result;
        UTF8Map utf8_map;
        if (!src_to_utf8)
        { // На входе UTF-8, а на выходе некоторая однобайтовая кодировка.
            conv_result = TranscodeFromUTF8(src_string_std, *dest_to_utf8);
        }
        else if (!dest_to_utf8)
        { // На входе однобайтовое представление, а на выходе нужно получить UTF-8.
            std::tuple<std::string, UTF8Map, UTF8Error> convex_result = TranscodeToUTF8Ex(src_string_std, *src_to_utf8);
            conv_result = {move(get<0>(convex_result)), get<2>(convex_result)};
            utf8_map = move(get<1>(convex_result));
        }
        else
        { // Наконец, на входе и выходе разные однобайтовые кодировки.
            conv_result = TranscodeBetweenUnibytes(src_string_std, *src_to_utf8, *dest_to_utf8);
        }
        if (conv_result.second.code != UTF8ErrorCode::UTF8_NO_ERROR)
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_STRING_ENCODING_ERROR,
                              "Ошибка перекодирования в положении "s + to_string(conv_result.second.pos));

        // Перекодировка прошла нормально, формируем и возвращаем полученный результат.
        runtime::String result_string(move(conv_result.first));
        result_string.encoding = dest_encoding;
        if (dest_encoding == UTF_8_ENCODING)
            result_string.utf8_map = move(utf8_map);
        return ObjectHolder::Own(move(result_string));
    }

    // Обобщённый поиск подстроки в строке, который для каждого конкретной разновидности отличается только передаваемой
    // поисковой функцией find_func. Эта функция применяется только для обработки строк в однобайтовых кодировках.
    ObjectHolder StringOpsInstance::MethodCommonFindUnibyte
        (const FindArgsT& args_values, ObjectHolder needle_holder, CommonFindFunc find_func, Context& context) const
    {
        runtime::String* arg_str_haystack = std::get<0>(args_values);
        if (arg_str_haystack->encoding == UTF_8_ENCODING)
            return {};  // Для многобайтовых строк эту функцию применять невозможно.

        // Перекодируем искомую подстроку в ту же кодировку, которую имеет строка-аргумент, в которой будет производиться поиск.
        needle_holder = ConvertTranscodeTo(needle_holder, context, arg_str_haystack->encoding);
        const std::string &arg_str_haystack_std = arg_str_haystack->GetValue(),
                          &arg_str_needle_std = needle_holder.TryAs<runtime::String>()->GetValue();
        size_t arg_pos = std::get<2>(args_values);

        // Все параметры предстоящей операции определены и проверены. Можно выполнять.
        return ObjectHolder::Own(runtime::Number(static_cast<int>((arg_str_haystack_std.*find_func)(arg_str_needle_std, arg_pos))));
    }

    // Метод извлечения стандартного набора аргументов функции поиска, у всех разновидностей которого этот набор одинаков
    // ("стог", "иголка" и начальная/конечная позиция поиска).
    StringOpsInstance::FindArgsT StringOpsInstance::ExtractFindParams
        (const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context, size_t default_pos) const
    {
        constexpr MethodParamCheckMode param_check_mode = static_cast<MethodParamCheckMode>
            (MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_GREATER_EQ | MethodParamCheckMode::PARAM_CHECK_TYPE_ONLY_FOR_MIN_ARGS);
        CheckMethodParams(context, method, param_check_mode, MethodParamType::PARAM_TYPE_STRING, 2, actual_args);
        if (actual_args.size() > 3) // Допускается от 2 до 3 параметров (включительно).
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT, "Метод " + method + " может принимать 2 или 3 параметра");

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

        return {actual_args[0].TryAs<runtime::String>(), actual_args[1].TryAs<runtime::String>(), arg_pos};
    }

    ObjectHolder StringOpsInstance::MethodSize(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        constexpr MethodParamCheckMode param_check_mode = static_cast<MethodParamCheckMode>
            (MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_GREATER_EQ | MethodParamCheckMode::PARAM_CHECK_TYPE_ONLY_FOR_MIN_ARGS);
        CheckMethodParams(context, "Size"s, param_check_mode, MethodParamType::PARAM_TYPE_STRING, 1, actual_args);
        if (actual_args.size() > 2) // Допускается 1 или 2 параметра метода.
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT, "Метод size может принимать 1 или 2 параметра");
        const runtime::String* arg_str = actual_args[0].TryAs<runtime::String>();
        const std::string& arg_str_std = arg_str->GetValue();

        // Опциональный второй параметр (если он есть) явно указывает способ вычисления длины строки-аргумента. Если он оценивается как "ИСТИНА",
        // то длина строки всегда вычисляется в многобайтовых UTF-8-кодах. Если оценка приводит к "ЛОЖЬ", то расчёт длины  выполняется в любом случае
        // в байтах (однобайтовых символах). Наконец, при отсутствии второго аргумента режим выбирается в зависимости от типа кодировки входной строки.
        bool is_utf8_size;
        if (actual_args.size() > 1) // Есть аргумент, явно указывающий "кодировочный" режим работы метода.
            is_utf8_size = runtime::IsTrue(actual_args[1]);
        else
            is_utf8_size = arg_str->encoding == UTF_8_ENCODING;

        if (is_utf8_size)
        { // Требуется вернуть размер строки в UTF-8-символах.
            if (arg_str->encoding == UTF_8_ENCODING)
                // Карта UTF-8-символов уже существует. Используем её размер как размер строки.
                return ObjectHolder::Own(runtime::Number(static_cast<int>(arg_str->SymbolSizeOf())));
            else
                // Карты местоположения UTF-8-символов в строке нет, так как она имеет однобайтовую кодировку. Создадим такую карту и вернём её длину.
                return ObjectHolder::Own(runtime::Number(static_cast<int>(BuildUTF8Map(arg_str_std).SymbolSizeOf())));
        }
        else
        { // Нужно вычислить длину строки в однобайтовых символа (то есть просто в байтах).
            return ObjectHolder::Own(runtime::Number(static_cast<int>(arg_str_std.size())));
        }
    }
    
    ObjectHolder StringOpsInstance::MethodConcat(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        CheckMethodParams(context, "Concat"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_GREATER_EQ,
                          MethodParamType::PARAM_TYPE_STRING, 1, actual_args);

        // Целевую кодировку формируемой суммарной строки примем равной той, которая применяется для первого аргумента метода.
        const SingleByteEncodingDesc* dest_encoding = actual_args[0].TryAs<runtime::String>()->encoding;
        std::string result_string;
        for (const ObjectHolder& arg_string_holder : actual_args)
            result_string += ConvertTranscodeTo(arg_string_holder, context, dest_encoding).TryAs<runtime::String>()->GetValue();

        runtime::String return_string(move(result_string));
        return_string.encoding = dest_encoding;
        if (dest_encoding == UTF_8_ENCODING)
            return_string.utf8_map = BuildUTF8Map(return_string.GetValue());

        return ObjectHolder::Own(move(return_string));
    }
    
    // append(arg_str_to, arg_str_what, arg_pos, arg_count) - присоединение к строке arg_str_to arg_count символов строки arg_str_what,
    // начиная с позиции arg_pos в ней.
    ObjectHolder StringOpsInstance::MethodAppend(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        constexpr MethodParamCheckMode param_check_mode = static_cast<MethodParamCheckMode>
            (MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_GREATER_EQ | MethodParamCheckMode::PARAM_CHECK_TYPE_ONLY_FOR_MIN_ARGS);
        CheckMethodParams(context, "Append"s, param_check_mode, MethodParamType::PARAM_TYPE_STRING, 2, actual_args);
        if (actual_args.size() > 4) // Допускается от 2 до 4 параметров (включительно).
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT, "Метод append может принимать от 2 до 4 параметров");
        
        // Выделяем из входных аргументов строку, к которой будет выполняться присоединение.
        const runtime::String* arg_str_to = actual_args[0].TryAs<runtime::String>();
        std::string arg_str_to_std = arg_str_to->GetValue();
        // Выделяем из входных аргументов строку, подстрока которой будет присоединяться к первому параметру метода,
        // а затем конвертируем её в ту же кодировку, что и строка-приёмник arg_str_to.
        ObjectHolder cnv_str_holder = ConvertTranscodeTo(actual_args[1], context, arg_str_to->encoding);
        const runtime::String* cnv_str_what = cnv_str_holder.TryAs<runtime::String>();        
        // Считываем положение и размер добавляемой подстроки.
        auto [arg_byte_pos, arg_byte_count] = ExtractPosSize(actual_args, 2, cnv_str_what, context);

        // Все параметры предстоящей операции определены и проверены. Можно выполнять. Если требуется, также пересоставим карту
        // распределения многобайтовых символов в полученной UTF-8-строке.
        runtime::String appended_str(move(arg_str_to_std.append(cnv_str_what->GetValue(), arg_byte_pos, arg_byte_count)));
        appended_str.encoding = arg_str_to->encoding;
        if (appended_str.encoding == UTF_8_ENCODING)
            appended_str.utf8_map = BuildUTF8Map(appended_str.GetValue());

        return ObjectHolder::Own(runtime::String(move(appended_str)));
    }

    // substr(arg_str, arg_pos, arg_length) - извлечение подстроки из строки arg_str длиной не более arg_length символов,
    // начиная с символа с индексом arg_pos.
    ObjectHolder StringOpsInstance::MethodSubstr(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        constexpr MethodParamCheckMode param_check_mode = static_cast<MethodParamCheckMode>
            (MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_GREATER_EQ | MethodParamCheckMode::PARAM_CHECK_TYPE_ONLY_FOR_MIN_ARGS);
        CheckMethodParams(context, "Substr"s, param_check_mode, MethodParamType::PARAM_TYPE_STRING, 1, actual_args);

        const runtime::String* arg_str = actual_args[0].TryAs<runtime::String>();
        const std::string& arg_str_std = arg_str->GetValue();
        auto [arg_pos, arg_count] = ExtractPosSize(actual_args, 1, arg_str, context);

        // Все параметры предстоящей операции определены и проверены. Можно выполнять. Для UTF-8-строки также составим для выделенной
        // из неё подстроки карту расположения UTF-8-символов в ней.
        runtime::String substr_val(arg_str_std.substr(arg_pos, arg_count));
        substr_val.encoding = arg_str->encoding;
        if (substr_val.encoding == UTF_8_ENCODING)
            substr_val.utf8_map = BuildUTF8Map(substr_val.GetValue());

        return ObjectHolder::Own(runtime::String(move(substr_val)));
    }

    // find(arg_str_haystack, arg_str_needle, arg_pos) - поиск первого вхождения подстроки arg_str_needle в строку arg_str_haystack,
    // начиная с позиции arg_pos.
    ObjectHolder StringOpsInstance::MethodFind(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        FindArgsT args_values = ExtractFindParams(method, actual_args, context, 0);
        runtime::String* arg_str_haystack = std::get<0>(args_values);
        if (arg_str_haystack->encoding != UTF_8_ENCODING)
            // Для однобайтовых строк используем соответствующую функцию из STL std::string.
            return MethodCommonFindUnibyte(args_values, actual_args[1], &std::string::find, context);

        // Если аргумент arg_str_haystack (в котором будет производиться поиск) имеет UTF-8-представление,
        // выполняем поисковую операцию самостоятельно. Сначала перекодируем искомую подстроку ("иголку") также в UTF-8,
        // а затем проверяем возможное наличие "иголки" по всем возможным начальным позициям в "стоге", начиная с arg_pos.
        ObjectHolder needle_holder = ConvertTranscodeTo(actual_args[1], context, arg_str_haystack->encoding);
        runtime::String* arg_str_needle = needle_holder.TryAs<runtime::String>();
        const std::string &arg_str_haystack_std = arg_str_haystack->GetValue(),
                          &arg_str_needle_std = arg_str_needle->GetValue();
        size_t arg_pos = std::get<2>(args_values),
               haystack_size = arg_str_haystack->SymbolSizeOf(),
               needle_size = arg_str_needle->SymbolSizeOf();

        if (needle_size > haystack_size || arg_pos > (haystack_size - needle_size))
            // Подстроку нужной длины с такой позиции arg_pos найти заведомо невозможно.
            return ObjectHolder::Own(runtime::Number(static_cast<int>(std::string::npos)));
        if (needle_size == 0)
            // Считаем, что пустая подстрока существует всегда и везде, то есть у любой строки с любой её существующей позиции.
            return ObjectHolder::Own(runtime::Number(static_cast<int>(arg_pos)));

        size_t needle_byte_size = arg_str_needle->BytePosAfterEnd();
        for (size_t i = arg_pos; i <= haystack_size - needle_size; ++i)
        {
            // Проверяем наличие подстроки arg_str_needle_std в строке arg_str_haystack_std, начиная с UTF-8-символа с индексом i.
            size_t haystack_substr_pos = arg_str_haystack->SymbolBytePos(i);
            if (CompareUTF8Substr(arg_str_haystack_std, haystack_substr_pos, needle_byte_size,
                                  arg_str_needle_std, 0, needle_byte_size) == 0)
                // Подстрока "иголка" обнаружилась в "стогу".
                return ObjectHolder::Own(runtime::Number(static_cast<int>(i)));
        }
        // Найти требуемую подстроку в указанной строке не удалось.
        return ObjectHolder::Own(runtime::Number(static_cast<int>(std::string::npos)));
    }
    
    // rfind(arg_str_haystack, arg_str_needle, arg_pos) - поиск последнего вхождения подстроки arg_str_needle в строку arg_str_haystack,
    // заканчивая поиск позицией arg_pos (то есть поиск проводится внутри префикса arg_str_haystack до символа arg_pos включительно).
    ObjectHolder StringOpsInstance::MethodRFind(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        FindArgsT args_values = ExtractFindParams(method, actual_args, context, (std::numeric_limits<size_t>::max)());
        runtime::String* arg_str_haystack = std::get<0>(args_values);
        if (arg_str_haystack->encoding != UTF_8_ENCODING)
            // Для однобайтовых строк используем соответствующую готовую функцию из STL std::string.
            return MethodCommonFindUnibyte(args_values, actual_args[1], &std::string::rfind, context);

        // Если аргумент arg_str_haystack (в котором будет производиться поиск) имеет UTF-8-представление,
        // используем другой способ выполнения операции. Сначала перекодируем искомую подстроку также в UTF-8, а затем сканируем "стог"
        // на предмет поиска символов из "иголки" своими собственными средствами.
        ObjectHolder needle_holder = ConvertTranscodeTo(actual_args[1], context, arg_str_haystack->encoding);
        runtime::String* arg_str_needle = needle_holder.TryAs<runtime::String>();
        const std::string &arg_str_haystack_std = arg_str_haystack->GetValue(),
                          &arg_str_needle_std = arg_str_needle->GetValue();
        size_t haystack_size = arg_str_haystack->SymbolSizeOf(),
               needle_size = arg_str_needle->SymbolSizeOf(),
               arg_pos = std::get<2>(args_values);

        if (needle_size == 0)
            // Считаем, что пустая подстрока существует всегда и везде, то есть у любой строки с любой её существующей позиции.
            return ObjectHolder::Own(runtime::Number(static_cast<int>(min(arg_pos, haystack_size))));
        if (needle_size > haystack_size)
            // Строка слишком короткая, в такой строке требуемой подстроки явно не существует.
            return ObjectHolder::Own(runtime::Number(static_cast<int>(std::string::npos)));

        size_t min_arg_pos = haystack_size - needle_size;
        if (arg_pos > min_arg_pos)
            arg_pos = min_arg_pos;
        size_t needle_byte_size = arg_str_needle->BytePosAfterEnd();
        while (true)
        {
            size_t haystack_substr_pos = arg_str_haystack->SymbolBytePos(arg_pos);
            if (CompareUTF8Substr(arg_str_haystack_std, haystack_substr_pos, needle_byte_size,
                arg_str_needle_std, 0, needle_byte_size) == 0)
                // Подстрока "иголка" обнаружилась в "стогу".
                return ObjectHolder::Own(runtime::Number(static_cast<int>(arg_pos)));

            if (arg_pos == 0)
                break;
            --arg_pos;
        }
        // Найти требуемую подстроку в указанной строке не удалось.
        return ObjectHolder::Own(runtime::Number(static_cast<int>(std::string::npos)));
    }
    
    // find_first_of(arg_str_haystack, arg_str_needles, arg_pos) - поиск первого вхождения любого символа строки arg_str_needles в
    // строку arg_str_haystack, начиная с позиции arg_pos.
    ObjectHolder StringOpsInstance::MethodFindFirstOf
        (const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        FindArgsT args_values = ExtractFindParams(method, actual_args, context, 0);
        runtime::String* arg_str_haystack = std::get<0>(args_values);
        if (arg_str_haystack->encoding != UTF_8_ENCODING)
            // Для однобайтовых строк используем соответствующую функцию из STL std::string.
            return MethodCommonFindUnibyte(args_values, actual_args[1], &std::string::find_first_of, context);

        // Если аргумент arg_str_haystack (в котором будет производиться поиск) имеет UTF-8-представление,
        // используем другой способ выполнения операции. Сначала перекодируем искомую подстроку также в UTF-8, а затем сканируем "стог"
        // на предмет поиска символов из "иголки" своими собственными средствами.
        ObjectHolder needles_holder = ConvertTranscodeTo(actual_args[1], context, arg_str_haystack->encoding);
        runtime::String* arg_str_needles = needles_holder.TryAs<runtime::String>();
        const std::string& haystack_std = arg_str_haystack->GetValue();
        size_t arg_pos = std::get<2>(args_values),
               haystack_size = arg_str_haystack->SymbolSizeOf();

        for (size_t i = arg_pos; i < haystack_size; ++i)
        {
            if (arg_str_needles->FindSymbol(haystack_std, arg_str_haystack->SymbolBytePos(i), arg_str_haystack->SymbolByteSize(i)) != std::string::npos)
                // Символ "стога" с индексом i обнаружился среди "иголок" arg_str_needles.
                return ObjectHolder::Own(runtime::Number(static_cast<int>(i)));
        }
        // Ни один символ из набора "иголок" arg_str_needles не найден в arg_str_haystack.
        return ObjectHolder::Own(runtime::Number(static_cast<int>(std::string::npos)));
    }
    
    // find_first_not_of(arg_str_haystack, arg_str_needle_list, arg_pos) - поиск первого вхождения любого символа не из строки arg_str_needle в
    // строку arg_str_haystack, начиная с позиции arg_pos.
    ObjectHolder StringOpsInstance::MethodFindFirstNotOf
        (const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        FindArgsT args_values = ExtractFindParams(method, actual_args, context, 0);
        runtime::String* arg_str_haystack = std::get<0>(args_values);
        if (arg_str_haystack->encoding != UTF_8_ENCODING)
            // Для однобайтовых строк используем соответствующую функцию из STL std::string.
            return MethodCommonFindUnibyte(args_values, actual_args[1], &std::string::find_first_not_of, context);

        // Если аргумент arg_str_haystack (в котором будет производиться поиск) имеет UTF-8-представление,
        // используем альтернативный метод выполнения операции собственными средствами.
        ObjectHolder needles_holder = ConvertTranscodeTo(actual_args[1], context, arg_str_haystack->encoding);
        runtime::String* arg_str_needles = needles_holder.TryAs<runtime::String>();
        const std::string& haystack_std = arg_str_haystack->GetValue();
        size_t arg_pos = std::get<2>(args_values),
               haystack_size = arg_str_haystack->SymbolSizeOf();

        for (size_t i = arg_pos; i < haystack_size; ++i)
        {
            if (arg_str_needles->FindSymbol(haystack_std, arg_str_haystack->SymbolBytePos(i), arg_str_haystack->SymbolByteSize(i)) == std::string::npos)
                // Обнаружен символ "стога" с индексом i, которого нет среди "иголок" arg_str_needles.
                return ObjectHolder::Own(runtime::Number(static_cast<int>(i)));
        }
        // В "стогу" arg_str_haystack нет ни одного символа, которого бы не было в наборе "иголок" arg_str_needles.
        return ObjectHolder::Own(runtime::Number(static_cast<int>(std::string::npos)));
    }
    
    // find_last_of(arg_str_haystack, arg_str_needle_list, arg_pos) - поиск последнего вхождения любого символа строки arg_str_needle в
    // строку arg_str_haystack, заканчивая поиск позицией arg_pos.
    ObjectHolder StringOpsInstance::MethodFindLastOf
        (const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        FindArgsT args_values = ExtractFindParams(method, actual_args, context, 0);
        runtime::String* arg_str_haystack = std::get<0>(args_values);
        if (arg_str_haystack->encoding != UTF_8_ENCODING)
            // Для однобайтовых строк используем соответствующую функцию из STL std::string.
            return MethodCommonFindUnibyte(args_values, actual_args[1], &std::string::find_last_of, context);

        // Если аргумент arg_str_haystack (в котором будет производиться поиск) имеет UTF-8-представление, используем другой способ
        // выполнения поисковой операции.
        ObjectHolder needles_holder = ConvertTranscodeTo(actual_args[1], context, arg_str_haystack->encoding);
        runtime::String* arg_str_needles = needles_holder.TryAs<runtime::String>();
        const std::string& haystack_std = arg_str_haystack->GetValue();
        size_t arg_pos = std::get<2>(args_values),
               haystack_size = arg_str_haystack->SymbolSizeOf();

        if (haystack_size > 0)
        {
            for (size_t i = haystack_size - 1; i >= arg_pos; --i)
            {
                if (arg_str_needles->FindSymbol(haystack_std, arg_str_haystack->SymbolBytePos(i), arg_str_haystack->SymbolByteSize(i)) != std::string::npos)
                    // Символ "стога" с индексом i обнаружился среди "иголок" arg_str_needles.
                    return ObjectHolder::Own(runtime::Number(static_cast<int>(i)));
                if (i == 0)
                    break;
            }
        }
        // Ни один символ из набора "иголок" arg_str_needles не найден в arg_str_haystack.
        return ObjectHolder::Own(runtime::Number(static_cast<int>(std::string::npos)));
    }

    // find_last_not_of(arg_str_haystack, arg_str_needle_list, arg_pos) - поиск последнего вхождения любого символа не из строки arg_str_needle в
    // строку arg_str_haystack, заканчивая поиск позицией arg_pos.
    ObjectHolder StringOpsInstance::MethodFindLastNotOf
        (const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        FindArgsT args_values = ExtractFindParams(method, actual_args, context, 0);
        runtime::String* arg_str_haystack = std::get<0>(args_values);
        if (arg_str_haystack->encoding != UTF_8_ENCODING)
            // Для однобайтовых строк используем соответствующую функцию из STL std::string.
            return MethodCommonFindUnibyte(args_values, actual_args[1], &std::string::find_last_not_of, context);

        // Если аргумент arg_str_haystack (в котором будет производиться поиск) имеет UTF-8-представление,
        // используем альтернативный метод выполнения операции собственными средствами.
        ObjectHolder needles_holder = ConvertTranscodeTo(actual_args[1], context, arg_str_haystack->encoding);
        runtime::String* arg_str_needles = needles_holder.TryAs<runtime::String>();
        const std::string& haystack_std = arg_str_haystack->GetValue();
        size_t arg_pos = std::get<2>(args_values),
               haystack_size = arg_str_haystack->SymbolSizeOf();

        if (haystack_size > 0)
        {
            for (size_t i = haystack_size - 1; i >= arg_pos; --i)
            {
                if (arg_str_needles->FindSymbol(haystack_std, arg_str_haystack->SymbolBytePos(i), arg_str_haystack->SymbolByteSize(i)) == std::string::npos)
                    // Обнаружен символ "стога" с индексом i, которого нет среди "иголок" arg_str_needles.
                    return ObjectHolder::Own(runtime::Number(static_cast<int>(i)));
                if (i == 0)
                    break;
            }
        }
        // В "стогу" arg_str_haystack нет ни одного символа, которого бы не было в наборе "иголок" arg_str_needles.
        return ObjectHolder::Own(runtime::Number(static_cast<int>(std::string::npos)));
    }
    
    // starts_with(arg_str_test, arg_str_start) - предикат, возвращающий "ИСТИНУ", если строка arg_str_test начинается с подстроки arg_str_start.
    ObjectHolder StringOpsInstance::MethodStartsWith(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        CheckMethodParams(context, "StartsWith"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_STRING, 2, actual_args);

        runtime::String* arg_str_haystack = actual_args[0].TryAs<runtime::String>();
        // Если требуется, транскодируем "иголку" в кодировку "стога".
        ObjectHolder arg_str_needle_holder = ConvertTranscodeTo(actual_args[1], context, arg_str_haystack->encoding);
        const std::string &arg_str_haystack_std = arg_str_haystack->GetValue(),
                          &arg_str_needle_std = arg_str_needle_holder.TryAs<runtime::String>()->GetValue();
        // Для однобайтовых кодировок используем стандартный starts_with и всё содержимое строк (их контейнеров). Для многобайтовых UTF-8 строк
        // используем только их начальные UTF-8-корректные части.
        bool starts_with_result;
        if (arg_str_haystack->encoding != UTF_8_ENCODING)
        {
            starts_with_result = arg_str_haystack_std.starts_with(arg_str_needle_std);
        }
        else
        {
            runtime::String* arg_str_needle = arg_str_needle_holder.TryAs<runtime::String>();
            if (arg_str_needle->SymbolSizeOf() > arg_str_haystack->SymbolSizeOf())
                return ObjectHolder::Own(runtime::Bool(false)); // Предполагаемый префикс слишком длинный, так что началом "стога" он быть явно не может.
            size_t needle_byte_length = arg_str_needle->BytePosAfterEnd();

            starts_with_result = (arg_str_haystack_std.compare(0, needle_byte_length, arg_str_needle_std, 0, needle_byte_length) == 0);
        }
        return ObjectHolder::Own(runtime::Bool(starts_with_result));
    }
    
    // ends_with(arg_str_test, arg_str_end) - предикат, возвращающий "ИСТИНУ", если строка arg_str_test заканчиватеся подстрокой arg_str_end.
    ObjectHolder StringOpsInstance::MethodEndsWith(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        CheckMethodParams(context, "EndsWith"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_STRING, 2, actual_args);

        runtime::String* arg_str_haystack = actual_args[0].TryAs<runtime::String>();
        // Если требуется, транскодируем "иголку" в кодировку "стога".
        ObjectHolder arg_str_needle_holder = ConvertTranscodeTo(actual_args[1], context, arg_str_haystack->encoding);
        const std::string &arg_str_haystack_std = arg_str_haystack->GetValue(),
                          &arg_str_needle_std = arg_str_needle_holder.TryAs<runtime::String>()->GetValue();
        // Для однобайтовых кодировок используем стандартный ends_with и всё содержимое строк (их контейнеров). Для многобайтовых UTF-8 строк
        // используем только их начальные UTF-8-корректные части.
        bool ends_with_result;
        if (arg_str_haystack->encoding != UTF_8_ENCODING)
        {
            ends_with_result = arg_str_haystack_std.ends_with(arg_str_needle_std);
        }
        else
        {
            runtime::String* arg_str_needle = arg_str_needle_holder.TryAs<runtime::String>();
            size_t haystack_byte_length = arg_str_haystack->BytePosAfterEnd(),                
                   needle_byte_length = arg_str_needle->BytePosAfterEnd();
            if (arg_str_needle->SymbolSizeOf() > arg_str_haystack->SymbolSizeOf() || needle_byte_length > haystack_byte_length)
                return ObjectHolder::Own(runtime::Bool(false)); // Предполагаемый суффикс слишком длинный, так что началом "стога" он быть явно не может.

            size_t haystack_suffix_pos = haystack_byte_length - needle_byte_length;
            ends_with_result = (arg_str_haystack_std.compare(haystack_suffix_pos, needle_byte_length, arg_str_needle_std, 0, needle_byte_length) == 0);
        }
        return ObjectHolder::Own(runtime::Bool(ends_with_result));
    }
    
    // contains(arg_str_haystack, arg_str_needle) - предикат, возвращающий "ИСТИНУ", если подстрока arg_str_needle входит в строку arg_str_haystack.
    ObjectHolder StringOpsInstance::MethodContains(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        CheckMethodParams(context, "Contains"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_STRING, 2, actual_args);

        std::string arg_str_haystack = actual_args[0].TryAs<runtime::String>()->GetValue(),
                    arg_str_needle = actual_args[1].TryAs<runtime::String>()->GetValue();
        return ObjectHolder::Own(runtime::Bool(arg_str_haystack.find(arg_str_needle) != std::string::npos));
    }
    
    // not_found() - возвращает константу, которой поисковые методы (...find...) сигнализируют о неудачном поиске (если найти искомый образец не удалось).
    ObjectHolder StringOpsInstance::MethodNotFound(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        CheckMethodParams(context, "NotFound"s, MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_ANY, 0, actual_args);

        return ObjectHolder::Own(runtime::Number(static_cast<int>(std::string::npos)));
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
        if (actual_args.size() >= 4)
        { // Явно задан arg_count.
            const runtime::Number* arg_count_ptr = actual_args[3].TryAs<runtime::Number>();
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
        constexpr MethodParamCheckMode param_check_mode = static_cast<MethodParamCheckMode>
            (MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_GREATER_EQ | MethodParamCheckMode::PARAM_CHECK_TYPE_ONLY_FOR_MIN_ARGS);
        CheckMethodParams(context, "Erase"s, param_check_mode, MethodParamType::PARAM_TYPE_STRING, 1, actual_args);
        if (actual_args.size() > 3) // Допускается от 1 до 3 параметров (включительно).
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT, "Метод Erase может принимать от 1 до 3 параметров");

        runtime::String* arg_str = actual_args[0].TryAs<runtime::String>();
        std::string arg_str_std = arg_str->GetValue();
        // Считываем байтовое положение и байтовую длину удаляемой подстроки с учётом типа используемой в строке-аргументе кодировки.
        auto [arg_byte_pos, arg_byte_length] = ExtractPosSize(actual_args, 1, arg_str, context);
        // Выполняем операцию. Если требуется, также пересоставим карту распределения многобайтовых символов в UTF-8-строке.
        runtime::String erased_str(move(arg_str_std.erase(arg_byte_pos, arg_byte_length)));
        if (arg_str->encoding == UTF_8_ENCODING)
            erased_str.utf8_map = BuildUTF8Map(erased_str.GetValue());

        return ObjectHolder::Own(move(erased_str));
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
        if (!arg_str_ptr)
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_TYPE, "Для метод Replace не заданы исходная строка");
        // Считываем из параметров и перекодируем заменяющую строку в кодировку принимающей.
        runtime::String* arg_str_ins_ptr = ConvertTranscodeTo(actual_args[3], context, arg_str_ptr->encoding).TryAs<runtime::String>();
        // Извлекаем из аргументов метода позиции и длины оперативных подстрок - удаляемой и заменяющей.
        auto [arg_pos, arg_count] = ExtractPosSize(actual_args, 1, arg_str_ptr, context);
        auto [arg_pos_ins, arg_count_ins] = ExtractPosSize(actual_args, 4, arg_str_ins_ptr, context);
        // Все параметры извлечены и проверены - можно выполнять операцию.
        std::string arg_str = arg_str_ptr->GetValue();
        const std::string& arg_str_ins = arg_str_ins_ptr->GetValue();
        runtime::String result_str(arg_str.replace(arg_pos, arg_count, arg_str_ins, arg_pos_ins, arg_count_ins));
        result_str.encoding = arg_str_ptr->encoding;
        if (arg_str_ptr->encoding == UTF_8_ENCODING)
            result_str.utf8_map = BuildUTF8Map(result_str.GetValue());

        return ObjectHolder::Own(move(result_str));
    }
    
    // replicate(arg_str, arg_count) - конструирование строки из arg_count копий строки arg_str.
    ObjectHolder StringOpsInstance::MethodReplicate(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        constexpr MethodParamCheckMode param_check_mode = static_cast<MethodParamCheckMode>
            (MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_GREATER_EQ | MethodParamCheckMode::PARAM_CHECK_TYPE_ONLY_FOR_MIN_ARGS);
        CheckMethodParams(context, "Replicate"s, param_check_mode, MethodParamType::PARAM_TYPE_STRING, 1, actual_args);
        if (actual_args.size() > 2) // Допускается 1 или 2 параметра.
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT, "Метод Replicate может принимать 1 или 2 параметра");

        std::string arg_str = actual_args[0].TryAs<runtime::String>()->GetValue();
        int arg_count = 1; // По умолчанию создаём одну копию аргумента.
        if (actual_args.size() >= 2)
        { // Явно указан arg_count.
            const runtime::Number* arg_count_ptr = actual_args[1].TryAs<runtime::Number>();
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
    
    // reverse(arg_str) - обращение (реверсирование) строки-аргумента arg_str.
    ObjectHolder StringOpsInstance::MethodReverse(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        CheckMethodParams(context, "Reverse"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_STRING, 1, actual_args);
        
        std::string arg_str = actual_args[0].TryAs<runtime::String>()->GetValue();
        std::reverse(arg_str.begin(), arg_str.end());
        return ObjectHolder::Own(runtime::String(move(arg_str)));
    }

    // asc(arg_str, arg_pos) - получение ASCII-кода символа строки arg_str, находящегося в позиции arg_pos.
    ObjectHolder StringOpsInstance::MethodAsc(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        constexpr MethodParamCheckMode param_check_mode = static_cast<MethodParamCheckMode>
            (MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_GREATER_EQ | MethodParamCheckMode::PARAM_CHECK_TYPE_ONLY_FOR_MIN_ARGS);
        CheckMethodParams(context, "Asc"s, param_check_mode, MethodParamType::PARAM_TYPE_STRING, 1, actual_args);
        if (actual_args.size() > 2) // Допускается 1 или 2 параметра.
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT, "Метод Asc может принимать 1 или 2 параметра");

        std::string arg_str = actual_args[0].TryAs<runtime::String>()->GetValue();
        int arg_pos = 0; // Значение начальной позиции интересующего нас символа по умолчанию.

        if (actual_args.size() >= 3)
        { // Явно задан arg_pos.
            const runtime::Number* arg_pos_ptr = actual_args[2].TryAs<runtime::Number>();
            if (!arg_pos_ptr)
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_TYPE, "Позиция в строке должна быть числом");
            arg_pos = arg_pos_ptr->GetIntValue();
            if (arg_pos < 0 || arg_pos >= static_cast<int>(arg_str.size()))
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_VALUE, "Задана недопустимая позиция в строке");
        }

        // Все параметры предстоящей операции определены и проверены. Можно выполнять.
        return ObjectHolder::Own(runtime::Number(arg_str[arg_pos]));
    }
    
    // chr(arg_code, arg_code, ...) - генерация строки из символов с ASCII-кодами arg_code.
    ObjectHolder StringOpsInstance::MethodChr(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        CheckMethodParams(context, "Chr"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_GREATER_EQ,
                          MethodParamType::PARAM_TYPE_NUMERIC, 1, actual_args);

        std::string result;
        for (size_t i = 0; i < actual_args.size(); ++i)
        {
            int code_value = actual_args[i].TryAs<runtime::Number>()->GetIntValue();
            if (code_value < 0 || code_value > 0xff)
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_TYPE, "Недопустимое значение ASCII-кода символа");
            result += code_value;
        }

        return ObjectHolder::Own(runtime::String(move(result)));
    }

    // utf8_char(arg_code, arg_code, ...) - генерация строки из символов с многобайтовыми кодами (в формате UTF-8) arg_code.
    ObjectHolder StringOpsInstance::MethodMbChr(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        CheckMethodParams(context, "UTF8Chr"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_GREATER_EQ,
                          MethodParamType::PARAM_TYPE_NUMERIC, 1, actual_args);

        std::string result_str;
        UTF8Map result_utf8_map;
        for (size_t i = 0; i < actual_args.size(); ++i)
        {
            uint32_t utf8_code_value = static_cast<uint32_t>(actual_args[i].TryAs<runtime::Number>()->GetIntValue());
            std::string utf8_code_str = ConvSymbToUTF8(utf8_code_value);
            if (utf8_code_str.size() > MAX_UNICODE_LENGTH)
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_STRING_ENCODING_ERROR, "Превышена максимальная длина UTF-8-кода символа");

            result_utf8_map.begin_map.push_back(result_str.size());
            result_utf8_map.last_symbol_size = utf8_code_str.size();
            result_str += utf8_code_str;
        }
        
        runtime::String utf8_result_str = move(result_str);
        utf8_result_str.utf8_map = move(result_utf8_map);
        utf8_result_str.encoding = UTF_8_ENCODING;
        return ObjectHolder::Own(move(utf8_result_str));
    }

    // utf8_asc(arg_str, arg_pos) - получение целочисленного многобайтового кода символа строки arg_str, находящегося в позиции arg_pos.
    // Предполагается, что строка-аргумент состоит из многобайтовых символов в кодировке UTF-8.
    // Если строка однобайтовая, позиция рассматривается как байтовый индекс (исчисляется в байтах от нуля) положения начального символа
    // гипотетического UTF-8-кода. Если же строка имеет кодировку UTF-8, то arg_pos - индекс некоторого UTF-8 (многобайтового) символа в ней,
    // UTF-8-код которого запрашивается.
    ObjectHolder StringOpsInstance::MethodMbAsc(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        constexpr MethodParamCheckMode param_check_mode = static_cast<MethodParamCheckMode>
            (MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_GREATER_EQ | MethodParamCheckMode::PARAM_CHECK_TYPE_ONLY_FOR_MIN_ARGS);
        CheckMethodParams(context, "UTF8Asc"s, param_check_mode, MethodParamType::PARAM_TYPE_STRING, 1, actual_args);
        if (actual_args.size() > 2) // Допускается 1 или 2 параметра.
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT, "Метод UTF8Asc может принимать 1 или 2 параметра");

        runtime::String* arg_str = actual_args[0].TryAs<runtime::String>();
        const std::string& arg_std = arg_str->GetValue();
        int arg_pos = 0; // Значение начальной позиции интересующего нас символа по умолчанию.

        if (actual_args.size() >= 3)
        { // Явно задан arg_pos.
            const runtime::Number* arg_pos_ptr = actual_args[2].TryAs<runtime::Number>();
            if (!arg_pos_ptr)
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_TYPE, "Позиция в строке должна быть числом");
            arg_pos = arg_pos_ptr->GetIntValue();
        }

        if (arg_str->encoding == UTF_8_ENCODING)
        { // Входная строка в UTF-8.
            if (arg_pos >= 0 && arg_pos < static_cast<int>(arg_str->utf8_map.begin_map.size()))
                arg_pos = static_cast<int>(arg_str->utf8_map.begin_map[arg_pos]);
            else
                arg_pos = -1;
        }
        if (arg_pos < 0 || arg_pos >= static_cast<int>(arg_std.size()))
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_VALUE, "Задана недопустимая позиция в строке");

        std::pair<uint32_t, size_t> conv_result_pair = ConvSymbFromUTF8(arg_std, arg_pos);
        last_unicode_ = conv_result_pair.first;
        return ObjectHolder::Own(runtime::Number(static_cast<int>(conv_result_pair.second)));
    }

    // to_number(arg_str, arg_pos, base_value) - преобразование в числовую форму фрагмента строки arg_str, начинающегося с arg_pos,
    // представляющего некоторое число в base_value - ичной системе счисления.
    ObjectHolder StringOpsInstance::MethodToNumber(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        constexpr MethodParamCheckMode param_check_mode = static_cast<MethodParamCheckMode>
            (MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_GREATER_EQ | MethodParamCheckMode::PARAM_CHECK_TYPE_ONLY_FOR_MIN_ARGS);
        CheckMethodParams(context, "ToNumber"s, param_check_mode, MethodParamType::PARAM_TYPE_STRING, 1, actual_args);
        if (actual_args.size() > 3) // Допускается от 1 до 3 параметров (включительно).
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT, "Метод ToNumber может принимать от 1 до 3 параметров");

        const runtime::String* arg_str = actual_args[0].TryAs<runtime::String>();
        const std::string& arg_str_std = arg_str->GetValue();
        // Значения фактических аргументов по умолчанию.
        size_t arg_pos = 0;
        int arg_radix = 0;

        if (actual_args.size() >= 2)
        { // Явно задан arg_pos.
            const runtime::Number* arg_pos_ptr = actual_args[1].TryAs<runtime::Number>();
            if (!arg_pos_ptr)
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_TYPE, "Позиция в строке должна быть числом");
            arg_pos = static_cast<size_t>(arg_pos_ptr->GetIntValue());
            if (arg_str->encoding == UTF_8_ENCODING)
            { // Для кодировки UTF-8 позиция в строке трактуется как номер многобайтового UTF-8-символа.
                if (arg_pos < arg_str->utf8_map.begin_map.size())
                    arg_pos = arg_str->utf8_map.begin_map[arg_pos];
                else
                    arg_pos = (std::numeric_limits<size_t>::max)();
            }
            if (arg_pos > arg_str_std.size())
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_VALUE, "Указанная позиция в строке недопустима");
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

        const char* begin_number_pos = arg_str_std.c_str() + arg_pos;
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
    
    // to_string(arg_number, base_value, double_precision) - преобразование в строку числового аргумента arg_number.
    // Для целых чисел строка формируется в base_value-ичной системе счисления.
    // Для дробных чисел аргумент base_value воспринимается как спецификатор формата вывода числа - научный, с фиксированной точкой
    // или шестнадцатеричный.
    //      В этом случае base_value == 1 - это целевой формат, эквивалентный std::chars_format::fixed (с фиксированной точкой).
    //                    base_value == 2 эквивалентно std::chars_format::scientific (научный формат).
    //                    base_value == 4 эквивалентно std::chars_format::hex (шестнадцатеричный).
    //      base_value здесь представляет собой битовую маску и указанные выше значения могут объединяться по ИЛИ.
    // Также для дробных чисел применяется третий аргумент - настройка точности double_precision - число знаков после запятой.
    ObjectHolder StringOpsInstance::MethodToString(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        static constexpr int CHARS_FORMAT_FIXED_MASK = 1;
        static constexpr int CHARS_FORMAT_SCIENTIFIC_MASK = 2;
        static constexpr int CHARS_FORMAT_HEX_MASK = 4;

        CheckMethodParams(context, "ToString"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_GREATER_EQ,
                          MethodParamType::PARAM_TYPE_NUMERIC, 1, actual_args);
        if (actual_args.size() > 3) // Допускается от 1 до 3 параметров (включительно).
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT, "Метод ToString может принимать от 1 до 3 параметров");
        
        const runtime::Number* arg_num = actual_args[0].TryAs<runtime::Number>();
        bool is_second_arg = actual_args.size() >= 2;
        int second_arg_val = 0;
        if (is_second_arg)
            second_arg_val = actual_args[1].TryAs<runtime::Number>()->GetIntValue();
        bool is_third_arg = actual_args.size() >= 3;
        int third_arg_val = 0;
        if (is_third_arg)
            third_arg_val = actual_args[2].TryAs<runtime::Number>()->GetIntValue();

        static constexpr size_t TEMP_BUFFER_LEN = 512;
        std::string temp_buffer(TEMP_BUFFER_LEN, '\0');
        std::to_chars_result conv_result;
        if (arg_num->IsDouble())
        {
            std::chars_format use_fmt = std::chars_format::general;
            if (is_second_arg)
            {
                use_fmt = static_cast<std::chars_format>(0);
                if (second_arg_val & CHARS_FORMAT_FIXED_MASK)
                    use_fmt |= std::chars_format::fixed;
                if (second_arg_val & CHARS_FORMAT_SCIENTIFIC_MASK)
                    use_fmt |= std::chars_format::scientific;
                if (second_arg_val & CHARS_FORMAT_HEX_MASK)
                    use_fmt |= std::chars_format::hex;
            }
            if (is_third_arg)
            {
                conv_result = std::to_chars(temp_buffer.data(), temp_buffer.data() + TEMP_BUFFER_LEN - 1, arg_num->GetDoubleValue(),
                                            use_fmt, third_arg_val);
            }
            else
            {
                conv_result = std::to_chars(temp_buffer.data(), temp_buffer.data() + TEMP_BUFFER_LEN - 1, arg_num->GetDoubleValue(),
                                            use_fmt);
            }
        }
        else
        {
            if (is_second_arg && (second_arg_val < 2 || second_arg_val > 36))
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_VALUE, "Задана недопустимая база преобразуемого числа");
            if (is_third_arg)
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT,
                                  "Для целых чисел метод ToString может принимать 1 или 2 параметра");
            int base_value = is_second_arg ? second_arg_val : 10; // По умолчанию используется десятичная система счисления.
            conv_result = std::to_chars(temp_buffer.data(), temp_buffer.data() + TEMP_BUFFER_LEN - 1, arg_num->GetIntValue(), base_value);
        }

        if (conv_result.ec != std::errc())
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_NUMBER_STRING_CONVERSION_ERROR,
                              "ToString : ошибка конверсии - " + std::to_string(static_cast<int>(conv_result.ec)));
        // Преобразование совершилось без ошибок - возвращаем его результат.
        return ObjectHolder::Own(runtime::String(temp_buffer.substr(0, conv_result.ptr - temp_buffer.data())));
    }

    // Получение кодировки, установленной для текстового (строкового) значения.
    ObjectHolder StringOpsInstance::MethodGetEncoding(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        CheckMethodParams(context, "GetEncoding"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_STRING, 1, actual_args);  // Единственный аргумент - строка для извлечения кодировки.

        const runtime::String* arg_input_str = actual_args[0].TryAs<runtime::String>();
        if (arg_input_str->encoding == nullptr)
            return ObjectHolder::Own(runtime::Number(NO_ENCODING_ID));      // Кодировка не установлена.
        if (arg_input_str->encoding == UTF_8_ENCODING)
            return ObjectHolder::Own(runtime::Number(UTF_8_ENCODING_ID));   // Используется UTF-8.

        auto encodings_data_it = std::find_if(::encodings_data.begin(), ::encodings_data.end(),
            [arg_input_str](const SingleByteEncodingDesc& encoding_desc) -> bool
            {
                return arg_input_str->encoding == &encoding_desc;
            });
        if (encodings_data_it != ::encodings_data.end())
            return ObjectHolder::Own(runtime::Number(static_cast<int>(encodings_data_it - ::encodings_data.begin() + 1)));

        return ObjectHolder::Own(runtime::Number(NON_INDEXED_ENCODING_ID));  // Установлена сторонняя, неиндексируемая кодировка.
    }
    
    // Назначение кодировки для текстового значения.
    ObjectHolder StringOpsInstance::MethodSetEncoding(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        constexpr MethodParamCheckMode param_check_mode = static_cast<MethodParamCheckMode>
            (MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_GREATER_EQ | MethodParamCheckMode::PARAM_CHECK_TYPE_ONLY_FOR_MIN_ARGS);
        // Первый аргумент - строка для назначения кодировки, второй аргумент - собственно, сама желаемая кодировка в виде её условного численного идентификатора.
        CheckMethodParams(context, "SetEncoding"s, param_check_mode, MethodParamType::PARAM_TYPE_STRING, 1, actual_args);
        if (actual_args.size() > 2) // Допускается 1 или 2 параметра.
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT, "Метод SetEncoding может принимать 1 или 2 параметра");

        runtime::String* arg_input_str = actual_args[0].TryAs<runtime::String>();
        int set_encoding_id = NO_ENCODING_ID;
        if (actual_args.size() > 1)
        {
            const ObjectHolder& encoding_holder = actual_args[1];
            if (encoding_holder.TryAs<runtime::Number>())
                set_encoding_id = CheckEncodingID(encoding_holder, context);
            else if (encoding_holder.TryAs<runtime::String>())
                set_encoding_id = CheckEncodingName(encoding_holder, context);
            else
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_TYPE, "Кодировка может быть задана либо числовым индексом, либо строковым именем");
        }

        if (set_encoding_id == NON_INDEXED_ENCODING_ID)
            set_encoding_id = NO_ENCODING_ID;
        arg_input_str->encoding = GetEncoding(set_encoding_id);
        if (set_encoding_id == UTF_8_ENCODING_ID) 
            // Включается многобайтовое представление UTF-8, требуется составить карту размещения символов в строке.
            arg_input_str->utf8_map = BuildUTF8Map(arg_input_str->GetValue());
        else
            // Устанавливается бескодировочный режим или назначается некоторая однобайтовая кодировка - карта расположения символов здесь не определена.
            arg_input_str->utf8_map.Clear();

        return ObjectHolder::Own(runtime::Number(set_encoding_id));
    }

    // Метод сравнения строк с возможностью явного указания кодировки или величин сравнительных весов символов.
    ObjectHolder StringOpsInstance::MethodCompare(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        // Первые два аргумента метода - сравниваемые строки.
        constexpr MethodParamCheckMode param_check_mode = static_cast<MethodParamCheckMode>
            (MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_GREATER_EQ | MethodParamCheckMode::PARAM_CHECK_TYPE_ONLY_FOR_MIN_ARGS);
        CheckMethodParams(context, "Compare"s, param_check_mode, MethodParamType::PARAM_TYPE_STRING, 2, actual_args);

        ObjectHolder real_first_arg = actual_args[0];
        ObjectHolder real_second_arg = actual_args[1];
        const runtime::String* first_compare_str = real_first_arg.TryAs<runtime::String>();
        const runtime::String* second_compare_str = real_second_arg.TryAs<runtime::String>();
        const runtime::String* compare_mode_str = first_compare_str;

        if (actual_args.size() > 2)
        { // Указан необязательный третий аргумент - режим сравнения. 
            compare_mode_str = actual_args[2].TryAs<runtime::String>();
            if (!compare_mode_str)
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_TYPE, "Аргумент режима должен быть строковым");
        }

        if (compare_mode_str->encoding != first_compare_str->encoding)
        { // Если кодировка сравнения задана в явном виде, конвертируем оба сравниваемых аргумента в неё. 
            real_first_arg = ConvertTranscodeTo(real_first_arg, context, compare_mode_str->encoding);
            first_compare_str = real_first_arg.TryAs<runtime::String>();
        }
        if (second_compare_str->encoding != first_compare_str->encoding)
        { // При несовпадении кодировок операндов сравнительной операции приводим второй операнд к кодировке первого.
            real_second_arg = ConvertTranscodeTo(real_second_arg, context, first_compare_str->encoding);
            second_compare_str = real_second_arg.TryAs<runtime::String>();
        }

        const std::string& op_str_1_std = first_compare_str->GetValue();
        const std::string& op_str_2_std = second_compare_str->GetValue();
        if (first_compare_str->encoding == UTF_8_ENCODING)
        {  // Строки для сравнения представлены в UTF-8.
            return ObjectHolder::Own(runtime::Number(CompareUTF8(op_str_1_std, op_str_2_std)));
        }
        else
        {   // Сравниваемые строки имеют однобайтовую кодировку.
            CompareCollateMode compare_mode
            {
                .upcase_table = compare_mode_str->GetUpcaseTable(),
                .collate = compare_mode_str->GetCollate(),
                .is_use_collate = compare_mode_str->is_use_collate,
                .is_equal_collate = compare_mode_str->is_equal_collate,
                .is_case_indep_compare = compare_mode_str->is_case_indep_compare
            };
            return ObjectHolder::Own(runtime::Number(CompareCollate(op_str_1_std, op_str_2_std, compare_mode)));
        }
    }

    // Перекодировка строк из одной кодировки в другую. Целевая кодировка выбирается указанием её условного номера вторым параметром метода.
    ObjectHolder StringOpsInstance::MethodEncTranscode(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        constexpr MethodParamCheckMode param_check_mode = static_cast<MethodParamCheckMode>
            (MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_GREATER_EQ | MethodParamCheckMode::PARAM_CHECK_TYPE_ONLY_FOR_MIN_ARGS);
        CheckMethodParams(context, "EncTranscode"s, param_check_mode, MethodParamType::PARAM_TYPE_STRING, 1, actual_args);
        if (actual_args.size() != 2) // Требуется строго 2 параметра - преобразуемая входная строка и желаемая целевая кодировка.
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT, "Метод EncTranscode требует 2 параметров");

        // Далее следует выбор целевой кодировки.
        const SingleByteEncodingDesc* dest_encoding;
        if (const runtime::String* encode_set_str = actual_args[1].TryAs<runtime::String>())
            dest_encoding = encode_set_str->encoding;
        else
            dest_encoding = &::encodings_data[static_cast<size_t>(CheckEncodingID(actual_args[1], context) - 1)];
        // Наконец, сама перекодировочная процедура и возврат результата.
        return ConvertTranscodeTo(actual_args[0], context, dest_encoding);
    }

    // Метод преобразует строку (свой первый аргумент) в массив целых чисел, каждый элемент которого равен коду соответствующего символа входной строки.
    // Для однобайтовых строк преобразование осуществляется прямо - "каждый байт в значение очередного элемента".
    // Для строк в кодировке UTF-8 каждый элемент массива устанавливается равным очередному многобайтовому Юникоду, выделенному из строки-аргумента.
    // Можно явно указать один из этих двух способов преобразования (независимо от кодировки входной строки) с помощью необязательного второго аргумента
    // метода. Если он имеется и равен True, то всегда применяется UTF-8-конверсия. Если же он есть и равен False, всегда используется побайтовое
    // преобразование.
    ObjectHolder StringOpsInstance::MethodToIntArray(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        constexpr MethodParamCheckMode param_check_mode = static_cast<MethodParamCheckMode>
            (MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_GREATER_EQ | MethodParamCheckMode::PARAM_CHECK_TYPE_ONLY_FOR_MIN_ARGS);
        CheckMethodParams(context, "ToIntArray"s, param_check_mode, MethodParamType::PARAM_TYPE_STRING, 1, actual_args);
        if (actual_args.size() > 2) // Допускается от 1 до 2 параметров (включительно).
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT, "Метод ToIntArray может принимать от 1 до 2 параметров");

        const runtime::String* arg_input_str = actual_args[0].TryAs<runtime::String>();
        bool is_from_utf8 = arg_input_str->encoding == UTF_8_ENCODING;
        const std::string& arg_input_std = arg_input_str->GetValue();

        if (actual_args.size() > 1) // Есть аргумент, явно указывающий "кодировочный" режим работы метода.
            is_from_utf8 = runtime::IsTrue(actual_args[1]);

        std::vector<uint32_t> output_codes;
        size_t input_str_pos = 0;
        while (input_str_pos < arg_input_std.size())
        {
            if (is_from_utf8)
            { // Входная строка имеет UTF-8-кодировку.
                std::pair<uint32_t, size_t> from_utf8_result = ConvSymbFromUTF8(arg_input_std, input_str_pos);
                if (from_utf8_result.second == 0)
                { // Ошибка извлечения из входной строки очередного UTF-8-юникода.
                    std::string err_mess =
                        ThrowMessages::ConstructThrowText("%1"s + std::to_string(input_str_pos), {ThrowMessageNumber::THRM_UTF8_EXTRACT_ERROR});
                    ThrowRuntimeError(context, ThrowMessageNumber::THRM_STRING_ENCODING_ERROR, err_mess);
                }
                last_unicode_ = from_utf8_result.first;
                output_codes.push_back(from_utf8_result.first);
                input_str_pos += from_utf8_result.second;
            }
            else
            { // Входная строка использует какую-либо однобайтовую кодировку.
                output_codes.push_back(static_cast<uint32_t>(arg_input_std[input_str_pos++]));
            }
        }

        runtime::ArrayInstance output_arr({static_cast<int>(output_codes.size())});
        for (size_t i = 0; i < output_codes.size(); ++i)
            output_arr.SetElement(i, ObjectHolder::Own(runtime::Number(static_cast<int>(output_codes[i]))));

        return ObjectHolder::Own(move(output_arr));
    }

    // Конструирует строку из целочисленного массива. Второй необязательный аргумент - желаемая кодировка. Если он не указан, кодировка определяется
    // автоматически по следующему принципу: если все значения массива лежат в закрытом диапазоне [0; 255], то создаётся однобайтовая строка без назначенной
    // кодировки. Если же хоть один элемент массива выходит за этот диапазон, то сконструированная строка будет иметь многобайтовую кодировку типа UTF-8.
    ObjectHolder StringOpsInstance::MethodFromIntArray(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        CheckMethodParams(context, "FromIntArray"s, MethodParamCheckMode::PARAM_CHECK_QUANTITY_GREATER_EQ,
                          MethodParamType::PARAM_TYPE_ANY, 1, actual_args);
        if (actual_args.size() > 2) // Допускается от 1 до 2 параметров (включительно).
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT, "Метод FromIntArray может принимать от 1 до 2 параметров");

        runtime::ArrayInstance* arg_src_array = actual_args[0].TryAs<runtime::ArrayInstance>(); // Массив-источник данных строки.
        if (!arg_src_array)
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_TYPE, "Первый параметр должен быть массивом Array");
        size_t arg_elems_count = arg_src_array->GetAbsoluteElementsCount();

        int encoding_id = 0;
        bool to_utf8_enc, enc_auto_select = actual_args.size() == 1;
        if (!enc_auto_select)
        { // Кодировка явно специфицирована вторым аргументом метода.
            encoding_id = CheckEncodingID(actual_args[1], context);
            if (encoding_id == NON_INDEXED_ENCODING_ID)
                // Указанный номер кодировки == NON_INDEXED_ENCODING_ID. Это также воспримем как требование выбрать выходную кодировку самостоятельно.
                enc_auto_select = true;
            else if (encoding_id == UTF_8_ENCODING_ID)            
                to_utf8_enc = true; // Явно требуется породить UTF-8-строку.
            else
                to_utf8_enc = false;  // Явно указана необходимость использовать однобайтовую кодировку.
        }

        if (enc_auto_select)
        { // Требуется выбрать целевую кодировку (UTF-8 или однобайтовую) автоматически.
            encoding_id = 0;
            to_utf8_enc = false;
            for (size_t i = 0; i < arg_elems_count; ++i)
            {
                const runtime::Number* arg_elem_i = arg_src_array->GetElement(i).TryAs<runtime::Number>();
                if (!arg_elem_i)
                    continue;
                int arg_elem_value = arg_elem_i->GetIntValue();
                if (arg_elem_value < 0 || arg_elem_value > 255)
                    to_utf8_enc = true;
            }
        }

        // Итак, целевая кодировка выбрана. Можно перейти к непосредственной генерации строки-результата.
        std::string result_str;
        UTF8Map utf8_map;
        for (size_t i = 0; i < arg_elems_count; ++i)
        {
            const runtime::Number* arg_elem_i = arg_src_array->GetElement(i).TryAs<runtime::Number>();
            if (!arg_elem_i)
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_TYPE, "Массив должен быть полностью числовым");
            int arg_elem_value = arg_elem_i->GetIntValue();
            if (to_utf8_enc)
            { // Создаём строку из многобайтовых UTF-8-кодов.
                utf8_map.begin_map.push_back(result_str.size());  // Параллельно с формированием строки ведём также карту расположения в ней UTF-8-кодов.
                std::string next_utf8_seq = ConvSymbToUTF8(static_cast<uint32_t>(arg_elem_value));
                utf8_map.last_symbol_size = next_utf8_seq.size();
                if (next_utf8_seq.size() > MAX_UNICODE_LENGTH)
                    ThrowRuntimeError(context, ThrowMessageNumber::THRM_STRING_ENCODING_ERROR, "Код UTF-8 превышает максимальную длину");

                result_str += next_utf8_seq;
            }
            else
            { // Формируем однобайтовую строку.
                result_str += static_cast<char>(max(0, min(arg_elem_value, 255)));
            }
        }

        runtime::String moufflon_result_str(std::move(result_str));
        // Установим для возвращаемой строки кодировочную информацию.
        if (to_utf8_enc)
        {
            moufflon_result_str.encoding = UTF_8_ENCODING;
            moufflon_result_str.utf8_map = std::move(utf8_map);
        }
        else if (encoding_id)
        {
            moufflon_result_str.encoding = &::encodings_data[static_cast<size_t>(encoding_id) - 1];
        }

        return ObjectHolder::Own(move(moufflon_result_str));
    }
    
    // -------- Методы установки соответствия между многобайтовыми UTF-8-символами и их однобайтовыми последовательностями в составе МУФЛОНОстроки.

    // Два следующих метода служат для получения информации из карты расположения UTF-8-кодов в UTF-8-кодированных строках.
    // Возврат положения (порядкового индекса) указанного UTF-8 символа в строке UTF-8. Возвращённый индекс - это положение стартового байта
    // UTF-8-кода запрошенного UTF-8-символа во входной строке, если рассматривать её как простой поток байтов.
    ObjectHolder StringOpsInstance::MethodMbSymPos(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        constexpr MethodParamCheckMode param_check_mode = static_cast<MethodParamCheckMode>
            (MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_GREATER_EQ | MethodParamCheckMode::PARAM_CHECK_TYPE_ONLY_FOR_MIN_ARGS);
        CheckMethodParams(context, "MbSymPos"s, param_check_mode, MethodParamType::PARAM_TYPE_STRING, 1, actual_args);
        if (actual_args.size() > 2) // Допускается 1 или 2 параметра.
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT, "Метод MbSymPos может принимать 1 или 2 параметра");

        const runtime::String* arg_input_str = actual_args[0].TryAs<runtime::String>();
        const std::string& arg_input_std = arg_input_str->GetValue();
        size_t symbol_index = 0;
        if (actual_args.size() > 1)
        {
            if (const runtime::Number* arg_set_encoding = actual_args[1].TryAs<runtime::Number>())
                symbol_index = static_cast<size_t>(arg_set_encoding->GetIntValue());
            else
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_TYPE, "Позиция в строке должна быть числом");
        }

        UTF8Map unibytes_utf8_map;
        const std::vector<size_t>* use_utf8_begin_map;
        if (arg_input_str->encoding == UTF_8_ENCODING)
        { // Это UTF-8-строка, она имеет готовую карту расположения кодов внутри неё.
            use_utf8_begin_map = &(arg_input_str->utf8_map.begin_map);
        }
        else
        { // Это однобайтовая строка, она такой карты не имеет и её нужно предварительно построить.
            unibytes_utf8_map = BuildUTF8Map(arg_input_std, symbol_index + 1);
            use_utf8_begin_map = &(unibytes_utf8_map.begin_map);
        }

        if (symbol_index < use_utf8_begin_map->size())
            return ObjectHolder::Own(runtime::Number(static_cast<int>((*use_utf8_begin_map)[symbol_index])));
        else
            return ObjectHolder::Own(runtime::Number(-1));
    }
    
    // Возврат размера в байтах указанного UTF-8 символа (размера его многобайтового UTF-8-кода) в UTF-8-закодированной строке.
    ObjectHolder StringOpsInstance::MethodMbSymSize(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        constexpr MethodParamCheckMode param_check_mode = static_cast<MethodParamCheckMode>
            (MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_GREATER_EQ | MethodParamCheckMode::PARAM_CHECK_TYPE_ONLY_FOR_MIN_ARGS);
        CheckMethodParams(context, "MbSymSize"s, param_check_mode, MethodParamType::PARAM_TYPE_STRING, 1, actual_args);
        if (actual_args.size() > 2) // Допускается 1 или 2 параметра.
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT, "Метод MbSymSize может принимать 1 или 2 параметра");

        const runtime::String* arg_input_str = actual_args[0].TryAs<runtime::String>();
        const std::string& arg_input_std = arg_input_str->GetValue();
        size_t symbol_index = 0;
        if (actual_args.size() > 1)
        {
            if (const runtime::Number* arg_set_encoding = actual_args[1].TryAs<runtime::Number>())
                symbol_index = static_cast<size_t>(arg_set_encoding->GetIntValue());
            else
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_TYPE, "Позиция в строке должна быть числом");
        }

        UTF8Map unibytes_utf8_map;
        const UTF8Map* use_utf8_map;
        if (arg_input_str->encoding == UTF_8_ENCODING)
        { // Это UTF-8-строка, она имеет готовую карту расположения кодов внутри неё.
            use_utf8_map = &(arg_input_str->utf8_map);
        }
        else
        { // Это однобайтовая строка, она такой карты не имеет и её нужно предварительно построить.
            unibytes_utf8_map = BuildUTF8Map(arg_input_std, symbol_index + 1);
            use_utf8_map = &unibytes_utf8_map;
        }

        size_t next_symbol_index = symbol_index + 1;
        if (symbol_index < use_utf8_map->begin_map.size() && next_symbol_index < use_utf8_map->begin_map.size())
            // Доступны запрошенный и следующий за ним символ.
            return ObjectHolder::Own(runtime::Number
                (static_cast<int>(use_utf8_map->begin_map[next_symbol_index] - use_utf8_map->begin_map[symbol_index])));
        else if (symbol_index < use_utf8_map->begin_map.size())
            // Запрошенный символ последний в строке.
            return ObjectHolder::Own(runtime::Number(static_cast<int>(use_utf8_map->last_symbol_size)));
        else // Запрошенного символа в строке не существует.
            return ObjectHolder::Own(runtime::Number(0));            
    }

    // Возврат размера в байтах некоторого UTF-8 символа, чьё представление начинается с указанной байтовой позиции (порядкового индекса).
    // Этот индекс есть именно байтовая позиция, то есть при её вычислении строка вне зависимости от её кодировки рассматривается как простая
    // неструктурированная последовательность байтов.
    ObjectHolder StringOpsInstance::MethodMbSymSizeAtPos(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        constexpr MethodParamCheckMode param_check_mode = static_cast<MethodParamCheckMode>
            (MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_GREATER_EQ | MethodParamCheckMode::PARAM_CHECK_TYPE_ONLY_FOR_MIN_ARGS);
        CheckMethodParams(context, "MbSymSizeAtPos"s, param_check_mode, MethodParamType::PARAM_TYPE_STRING, 1, actual_args);
        if (actual_args.size() > 2) // Допускается 1 или 2 параметра.
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT, "Метод MbSymSizeAtPos может принимать 1 или 2 параметра");

        const runtime::String* arg_input_str = actual_args[0].TryAs<runtime::String>();
        const std::string& input_str = arg_input_str->GetValue();

        size_t symbol_pos = 0;
        if (actual_args.size() > 1)
        {
            if (const runtime::Number* arg_set_encoding = actual_args[1].TryAs<runtime::Number>())
                symbol_pos = static_cast<size_t>(arg_set_encoding->GetIntValue());
            else
                ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_TYPE, "Позиция в строке должна быть числом");
        }
        if (symbol_pos >= input_str.size())
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAM_VALUE, "Указанное положение находится за пределами строки");

        std::pair<uint32_t, size_t> conv_result_pair = ConvSymbFromUTF8(input_str, symbol_pos);
        last_unicode_ = conv_result_pair.first;
        return ObjectHolder::Own(runtime::Number(static_cast<int>(conv_result_pair.second)));
    }

    // -------- 

    // Возврат Юникода последнего UTF-8 символа, который был обработан некоторыми операциями над многобайтовыми строками (в частности,
    // методом MethodMbSymSizeAtPos()).
    ObjectHolder StringOpsInstance::MethodLastMbSymCode(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        CheckMethodParams(context, "FromIntArray"s, MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_ANY, 0, actual_args);
        return ObjectHolder::Own(runtime::Number(static_cast<int>(last_unicode_)));
    }

    // Возврат константного условного номера кодировки, соответствующего UTF-8-представлению строк.
    ObjectHolder StringOpsInstance::MethodUTF8EncodingID(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        CheckMethodParams(context, "FromIntArray"s, MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_ANY, 0, actual_args);

        return ObjectHolder::Own(runtime::Number(UTF_8_ENCODING_ID));
    }

    // Возврат константного условного номера кодировки, соответствующего её отсутствию (неназначенной кодировке).
    ObjectHolder StringOpsInstance::MethodNoneEncodingID(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        CheckMethodParams(context, "FromIntArray"s, MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_ANY, 0, actual_args);

        return ObjectHolder::Own(runtime::Number(NO_ENCODING_ID));
    }

    void StringOpsInstance::Print(std::ostream& os, Context& context)
    {
        os << "StringOps:Erros" << last_to_number_error_ << ":Length:" << last_to_number_length_;
    }

    ObjectHolder StringOpsInstance::Call
        (const std::string& method_name, const std::vector<ObjectHolder>& actual_args, Context& context, const std::string& parent_name)
    {
        if (string_ops_method_table_.contains(method_name))
            return (this->*string_ops_method_table_.at(method_name))(method_name, actual_args, context);
        else
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_METHOD_NOT_FOUND);
    }
    
    bool StringOpsInstance::HasMethod(const std::string& method_name, size_t argument_count, const std::string& parent_name) const
    {
        if (!parent_name.empty())
            return false;

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

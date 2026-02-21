
#include "error_classes.h"
#include "parse.h"

using namespace std;

namespace runtime
{
    [[noreturn]] void ThrowRuntimeError(const ProgramCommandDescriptor& command_desc, ThrowMessageNumber msg_num, const std::string& except_text)
    {
        switch (msg_num)
        {
        // Различные типы несогласованности параметров по типу или количеству.
        case ThrowMessageNumber::THRM_ARRAY_MUST_HAVE_DIMS:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_MAP_CTOR_HAS_NO_PARAMS:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_MATH_CTOR_HAS_NO_PARAMS:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_STR_HAS_ONE_PARAM:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_SHIFT_INVALID_PARAMS:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_NOT_DIGIT_SIZES:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_ARGUMENTS:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_DEMAND_EQUAL:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_DEMAND_LESS_OR_EQUAL:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_DEMAND_GREATER_OR_EQUAL:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_PARAMETER:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_OF_METHOD:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_HAVE_INCOMPATIBLE_TYPE:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_DEMAND_ONE_ARGUMENT:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_FIRST_PARAM_OF_METHOD:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_MUST_BE_ITERATOR:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_PARAMS_TYPE_INCONSISTENCY:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_IN_METHOD:
            throw RuntimeError(ObjectHolder::Own(ErrorParamsInconsistency(msg_num, command_desc, except_text)));
        // Синтаксические ошибки.
        case ThrowMessageNumber::THRM_NOT_SUPPORT_FREE_FUNCTION:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_UNKNOWN_METHOD_CALL:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_METHOD_NOT_FOUND:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_BASE_CLASS:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_NOT_FOUND_FOR_CLASS:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_CLASS:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_ALREADY_EXISTS:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_VARIABLE_NOT_FOUND:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_INCORRECT_TOKEN_LIST:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_METHOD:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_QUALIFIER_NOT_ANCESTOR:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_AMBIGUOUS_OVERLOAD:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_METHOD_NOT_COROUTINE:
            throw RuntimeError(ObjectHolder::Own(SyntaxError(msg_num, command_desc, except_text)));
        // Неверная работа со ссылками.
        case ThrowMessageNumber::THRM_POINTER_RET_TO_VAL_DENIED:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_POINTER_RET_TOL_LOCAL_VAR_DENIED:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_INDIRECT_ASSIGN_ERROR:
            throw RuntimeError(ObjectHolder::Own(ReferenceError(msg_num, command_desc, except_text)));
        // Деление на нуль.
        case ThrowMessageNumber::THRM_DIVISION_BY_ZERO:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_MODULO_DIVISION_BY_ZERO:
            throw RuntimeError(ObjectHolder::Own(ErrorDivisionByZero(msg_num, command_desc, except_text)));
        // Неопределённые или недопустимые операции.
        case ThrowMessageNumber::THRM_IMPOSSIBLE_ADDITION:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_IMPOSSIBLE_SUBTRACTION:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_IMPOSSIBLE_MULTIPLICATION:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_IMPOSSIBLE_DIVISION:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_IMPOSSIBLE_COMPARE_EQUAL:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_IMPOSSIBLE_COMPARE_LESS:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_IMPOSSIBLE_MODULO_DIVISION:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_INVALID_PARAM_VALUE:
            throw RuntimeError(ObjectHolder::Own(DomainError(msg_num, command_desc, except_text)));
        // Некорректная работа со встроенными классами.
        case ThrowMessageNumber::THRM_INVALID_ARRAY_INDEX:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_PUSH_BACK_ONE_DIM_ONLY:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_BACK_ONE_DIM_ONLY:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_POP_BACK_ONE_DIM_ONLY:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_ARRAY_IS_EMPTY:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_ITERATOR_IN_PROGRESS_INSERT:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_ITERATOR_IN_PROGRESS_ERASE:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_ITERATOR_INVALID:
            throw RuntimeError(ObjectHolder::Own(LogicError(msg_num, command_desc, except_text)));
        // Неверная работа с подключением внешних модулей - в исходниках или двоичных("втыкал").
        case ThrowMessageNumber::THRM_INVALID_IMPORT_FILENAME:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_DYNAMIC_LIBRARY_NOT_LOADED:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_CREATE_PLUGIN_NOT_FOUND:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_LOAD_PLUGIN_LIST_NOT_FOUND:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_INCLUDE_INVALID_PARAMS:
            throw RuntimeError(ObjectHolder::Own(ModuleError(msg_num, command_desc, except_text)));
        // Прочие ошибки.
        case ThrowMessageNumber::THRM_RAISE_CALL:
            [[fallthrough]];
        case ThrowMessageNumber::THRM_URGENT_TERMINATE:
            [[fallthrough]];
        default:
            throw RuntimeError(ObjectHolder::Own(CommonError(msg_num, command_desc, except_text)));
        }
    }

    [[noreturn]] void ThrowRuntimeError(runtime::Executable* exec_obj_ptr, ThrowMessageNumber msg_num, const std::string& except_text)
    {
        ThrowRuntimeError(exec_obj_ptr->GetCommandDesc(), msg_num, except_text);
    }

    [[noreturn]] void ThrowRuntimeError(Context& context, ThrowMessageNumber msg_num, const std::string& except_text)
    {
        ThrowRuntimeError(context.GetLastCommandDesc(), msg_num, except_text);
    }

    [[noreturn]] void RethrowRuntimeError(Context& context, const TempError& orig_temp_error)
    {
        ThrowRuntimeError(context, orig_temp_error.msg_num, orig_temp_error.except_text);
    }

    const unordered_map<string_view, CommonError::CommonErrorCallMethod> CommonError::common_error_method_table_
    {
        {"get_text"sv, &CommonError::MethodGetText},
        {"GetText"sv, &CommonError::MethodGetText},
        {"get_full_text"sv, &CommonError::MethodGetFullText},
        {"GetFullText"sv, &CommonError::MethodGetFullText},
        {"get_error_code"sv, &CommonError::MethodGetErrorCode},
        {"GetErrorCode"sv, &CommonError::MethodGetErrorCode},
        {"get_module_id"sv, &CommonError::MethodGetModuleId},
        {"GetModuleId"sv, &CommonError::MethodGetModuleId},
        {"get_string_number"sv, &CommonError::MethodGetStringNumber},
        {"GetStringNumber"sv, &CommonError::MethodGetStringNumber}
    };

    bool CommonError::HasMethod(const std::string& method_name, size_t argument_count) const
    { // Все методы класса CommonError не имеют аргументов (имеют 0 аргументов).
        return argument_count == 0 && common_error_method_table_.count(method_name) != 0;
    }

    ObjectHolder CommonError::Call
        (const std::string& method_name, const std::vector<ObjectHolder>& actual_args, Context& context, const std::string& parent_name)
    {
        if (actual_args.size() == 0 && common_error_method_table_.count(method_name) != 0)
            return (this->*common_error_method_table_.at(method_name))(method_name, actual_args, context);
        else
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_METHOD_NOT_FOUND);
    }

    std::string CommonError::GetFullText() const
    {
        std::string error_full_text;
        if (command_desc_.module_id != INT_MAX)
            error_full_text = std::to_string(command_desc_.module_id) + '(' + std::to_string(command_desc_.module_string_number) + ')';
        if (msg_num_ != ThrowMessageNumber::THRM_UNKNOWN)
            error_full_text += ':' + ThrowMessages::GetThrowText(msg_num_);
        if (!msg_text_.empty())
            error_full_text += " - " + msg_text_;
        return error_full_text;
    }

    ObjectHolder CommonError::MethodGetText(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        return ObjectHolder::Own(String(GetText()));
    }

    ObjectHolder CommonError::MethodGetFullText(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        return ObjectHolder::Own(String(GetFullText()));
    }

    ObjectHolder CommonError::MethodGetErrorCode(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        return ObjectHolder::Own(Number(static_cast<int>(GetMsgNum())));
    }

    ObjectHolder CommonError::MethodGetModuleId(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        return ObjectHolder::Own(Number(command_desc_.module_id));
    }

    ObjectHolder CommonError::MethodGetStringNumber(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        return ObjectHolder::Own(Number(command_desc_.module_string_number));
    }

    NewCommonError::NewCommonError(std::vector<std::unique_ptr<Statement>> args) : args_(move(args))
    {
        if (args_.size() > 2) // Класс ошибки всегда имеет не более двух аргументов - тип и дополнительное сопровождающее сообщение.
            throw ParseError(ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT);
    }

    std::pair< ThrowMessageNumber, std::string> NewCommonError::CountParams(runtime::Closure& closure, runtime::Context& context)
    {
        ThrowMessageNumber msg_num = ThrowMessageNumber::THRM_UNKNOWN;
        string except_text;
        if (args_.size())
        { // Первый аргумент - условный номер ошибки. Число, указывающее на какой-либо член перечисления ThrowMessageNumber.
            ObjectHolder throw_message_number_object = args_[0]->Execute(closure, context);
            if (runtime::Number* throw_message_number_ptr = throw_message_number_object.TryAs<runtime::Number>())
                msg_num = static_cast<ThrowMessageNumber>(throw_message_number_ptr->GetIntValue());
            else
                ThrowRuntimeError(this, ThrowMessageNumber::THRM_PARAMS_TYPE_INCONSISTENCY);
        }
        if (args_.size() > 1)
        { // Второй аргумент - сопровождающий пояснительный текст.
            ObjectHolder except_text_object = args_[0]->Execute(closure, context);
            if (runtime::String* except_text_ptr = except_text_object.TryAs<runtime::String>())
                except_text = except_text_ptr->GetValue();
            else
                ThrowRuntimeError(this, ThrowMessageNumber::THRM_PARAMS_TYPE_INCONSISTENCY);
        }
        return {msg_num,  move(except_text)};
    }

    // Возвращает объект, содержащий значение типа CommonError - экземпляр класса ошибки CommonError.
    runtime::ObjectHolder NewCommonError::Execute(runtime::Closure& closure, runtime::Context& context)
    {
        PrepareExecute(this, closure, context);
        std::pair<ThrowMessageNumber, string> error_params = CountParams(closure, context);

        return ObjectHolder::Own(runtime::CommonError(error_params.first, this->GetCommandDesc(), error_params.second));
    }

    // Возвращает объект, содержащий значение типа ErrorDivisionByZero - экземпляр класса ошибки ErrorDivisionByZero.
    runtime::ObjectHolder NewErrorDivisionByZero::Execute(runtime::Closure& closure, runtime::Context& context)
    {
        PrepareExecute(this, closure, context);
        std::pair<ThrowMessageNumber, string> error_params = CountParams(closure, context);

        return ObjectHolder::Own(runtime::ErrorDivisionByZero(error_params.first, this->GetCommandDesc(), error_params.second));
    }

    // Возвращает объект, содержащий значение типа OverflowError - экземпляр класса ошибки OverflowError.
    runtime::ObjectHolder NewOverflowError::Execute(runtime::Closure& closure, runtime::Context& context)
    {
        PrepareExecute(this, closure, context);
        std::pair<ThrowMessageNumber, string> error_params = CountParams(closure, context);

        return ObjectHolder::Own(runtime::OverflowError(error_params.first, this->GetCommandDesc(), error_params.second));
    }

    // Возвращает объект, содержащий значение типа DomainError - экземпляр класса ошибки DomainError.
    runtime::ObjectHolder NewDomainError::Execute(runtime::Closure& closure, runtime::Context& context)
    {
        PrepareExecute(this, closure, context);
        std::pair<ThrowMessageNumber, string> error_params = CountParams(closure, context);

        return ObjectHolder::Own(runtime::DomainError(error_params.first, this->GetCommandDesc(), error_params.second));
    }

    // Возвращает объект, содержащий значение типа ErrorParamsInconsistency - экземпляр класса ошибки ErrorParamsInconsistency.
    runtime::ObjectHolder NewErrorParamsInconsistency::Execute(runtime::Closure& closure, runtime::Context& context)
    {
        PrepareExecute(this, closure, context);
        std::pair<ThrowMessageNumber, string> error_params = CountParams(closure, context);

        return ObjectHolder::Own(runtime::ErrorParamsInconsistency(error_params.first, this->GetCommandDesc(), error_params.second));
    }

    // Возвращает объект, содержащий значение типа SyntaxError - экземпляр класса ошибки SyntaxError.
    runtime::ObjectHolder NewSyntaxError::Execute(runtime::Closure& closure, runtime::Context& context)
    {
        PrepareExecute(this, closure, context);
        std::pair<ThrowMessageNumber, string> error_params = CountParams(closure, context);

        return ObjectHolder::Own(runtime::SyntaxError(error_params.first, this->GetCommandDesc(), error_params.second));
    }

    // Возвращает объект, содержащий значение типа ModuleError - экземпляр класса ошибки ModuleError.
    runtime::ObjectHolder NewModuleError::Execute(runtime::Closure& closure, runtime::Context& context)
    {
        PrepareExecute(this, closure, context);
        std::pair<ThrowMessageNumber, string> error_params = CountParams(closure, context);

        return ObjectHolder::Own(runtime::ModuleError(error_params.first, this->GetCommandDesc(), error_params.second));
    }

    // Возвращает объект, содержащий значение типа LogicError - экземпляр класса ошибки LogicError.
    runtime::ObjectHolder NewLogicError::Execute(runtime::Closure& closure, runtime::Context& context)
    {
        PrepareExecute(this, closure, context);
        std::pair<ThrowMessageNumber, string> error_params = CountParams(closure, context);

        return ObjectHolder::Own(runtime::LogicError(error_params.first, this->GetCommandDesc(), error_params.second));
    }

    // Возвращает объект, содержащий значение типа ReferenceError - экземпляр класса ошибки ReferenceError.
    runtime::ObjectHolder NewReferenceError::Execute(runtime::Closure& closure, runtime::Context& context)
    {
        PrepareExecute(this, closure, context);
        std::pair<ThrowMessageNumber, string> error_params = CountParams(closure, context);

        return ObjectHolder::Own(runtime::ReferenceError(error_params.first, this->GetCommandDesc(), error_params.second));
    }
} // namespace runtime

namespace ast
{
    unique_ptr<Statement> CreateCommonError(std::vector<std::unique_ptr<Statement>> args)
    {
        return make_unique<runtime::NewCommonError>(runtime::NewCommonError(move(args)));
    }

    unique_ptr<Statement> CreateErrorDivisionByZero(std::vector<std::unique_ptr<Statement>> args)
    {
        return make_unique<runtime::NewErrorDivisionByZero>(runtime::NewErrorDivisionByZero(move(args)));
    }

    unique_ptr<Statement> CreateOverflowError(std::vector<std::unique_ptr<Statement>> args)
    {
        return make_unique<runtime::NewOverflowError>(runtime::NewOverflowError(move(args)));
    }

    unique_ptr<Statement> CreateDomainError(std::vector<std::unique_ptr<Statement>> args)
    {
        return make_unique<runtime::NewDomainError>(runtime::NewDomainError(move(args)));
    }

    unique_ptr<Statement> CreateErrorParamsInconsistency(std::vector<std::unique_ptr<Statement>> args)
    {
        return make_unique<runtime::NewErrorParamsInconsistency>(runtime::NewErrorParamsInconsistency(move(args)));
    }

    unique_ptr<Statement> CreateSyntaxError(std::vector<std::unique_ptr<Statement>> args)
    {
        return make_unique<runtime::NewSyntaxError>(runtime::NewSyntaxError(move(args)));
    }

    unique_ptr<Statement> CreateModuleError(std::vector<std::unique_ptr<Statement>> args)
    {
        return make_unique<runtime::NewModuleError>(runtime::NewModuleError(move(args)));
    }

    unique_ptr<Statement> CreateLogicError(std::vector<std::unique_ptr<Statement>> args)
    {
        return make_unique<runtime::NewLogicError>(runtime::NewLogicError(move(args)));
    }

    unique_ptr<Statement> CreateReferenceError(std::vector<std::unique_ptr<Statement>> args)
    {
        return make_unique<runtime::NewReferenceError>(runtime::NewReferenceError(move(args)));
    }
}

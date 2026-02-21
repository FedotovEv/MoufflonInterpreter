#pragma once

#include "declares.h"
#include "throw_messages.h"
#include "runtime.h"
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <map>
#include <variant>

namespace runtime
{
    class TempError : public std::exception
    { // Временный класс ошибки, используемый тогда, когда внутрипрограммные координаты сбойного оператора пока неизвестны.
    public:
        TempError(ThrowMessageNumber p_msg_num, const std::string& p_except_text) :
            msg_num(p_msg_num), except_text(p_except_text)
        {}

        ThrowMessageNumber msg_num;
        std::string except_text;
    };

    // Основные классы ошибок, которые могут выбрасывать при своей работе внутренние функциии и классы окружения
    // периода исполнения программы на Муфлоне, которую обеспечивает для неё данный интерпретатор.
    class CommonError : public CommonClassInstance
    { // Общий класс ошибки, являющийся предком всех прочих таких классов, а также использующийся в том случае, если более
      // глубокая детализация ошибки невозможна.
    public:
        static constexpr const char* COMMON_ERROR_CLASS_NAME = "CommonError";
        using CommonErrorCallMethod = ObjectHolder(CommonError::*)(const std::string&, const std::vector<ObjectHolder>&,
                                                                   Context&);
        CommonError(ThrowMessageNumber msg_num = ThrowMessageNumber::THRM_UNKNOWN, const ProgramCommandDescriptor& command_desc = {},
                    const std::string& msg_text = {}) :
            msg_num_(msg_num), command_desc_(command_desc), msg_text_(msg_text)
        {}
        CommonError(const CommonError&) = default;
        CommonError(CommonError&&) = default;
        //
        CommonError& operator=(const CommonError&) = default;
        CommonError& operator=(CommonError&&) = default;

        bool HasMethod(const std::string& method_name, size_t argument_count) const override;
        ObjectHolder Call(const std::string& method_name, const std::vector<ObjectHolder>& actual_args,
                          Context& context, const std::string& parent_name = {}) override;
        std::string GetClassName() const override // Возвращает имя данного класса.
        {
            return COMMON_ERROR_CLASS_NAME;
        }

        // Классы ошибок имеют двухуровневую иерархию - родоначальник класс CommonError (с именем COMMON_ERROR_CLASS_NAME)
        // и один уровень производных от него классов. Поэтому среди наших предков могуит быть либо класс CommonError, 
        [[nodiscard]] bool IsSuccessorOf(const std::string& test_my_parent) const override
        {
            return COMMON_ERROR_CLASS_NAME == test_my_parent || GetClassName() == test_my_parent;
        }
        [[nodiscard]] bool IsSuccessorOf(const Class& test_my_parent) const override
        {
            return COMMON_ERROR_CLASS_NAME == test_my_parent.GetName() || GetClassName() == test_my_parent.GetName();
        }

        const std::string& GetText() const
        {
            return msg_text_;
        }
        std::string GetFullText() const;
        // Выводит в os код и текст ошибки в виде строки.
        void Print(std::ostream& os, Context& context) override
        {
            os << GetFullText();
        }

        ProgramCommandDescriptor& GetProgramCommandDescriptor()
        {
            return command_desc_;
        }

        const ProgramCommandDescriptor& GetProgramCommandDescriptor() const
        {
            return command_desc_;
        }

        ThrowMessageNumber GetMsgNum() const
        {
            return msg_num_;
        }

        CommonError& SetMsgNum(ThrowMessageNumber msg_num)
        {
            msg_num_ = msg_num;
            return *this;
        }

        CommonError& SetText(std::string msg_text)
        {
            msg_text_ = std::move(msg_text);
            return *this;
        }

    protected:
        static const std::unordered_map<std::string_view, CommonErrorCallMethod> common_error_method_table_;

        ProgramCommandDescriptor command_desc_;
        ThrowMessageNumber msg_num_ = ThrowMessageNumber::THRM_UNKNOWN;
        std::string msg_text_;

        // Обработчики методов всех классов ошибки, доступные для вызова из программы на Муфлоне. Их назначение - информирование
        // программы о сущности произошедшего события.
        ObjectHolder MethodGetText(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
        ObjectHolder MethodGetFullText(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
        ObjectHolder MethodGetErrorCode(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
        ObjectHolder MethodGetModuleId(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
        ObjectHolder MethodGetStringNumber(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    };

    class ErrorDivisionByZero : public CommonError
    { // Класс ошибки деления на нуль.
    public:
        using CommonError::CommonError;
        std::string GetClassName() const override // Возвращает имя данного класса.
        {
            return "ErrorDivisionByZero";
        }
    };

    class OverflowError : public CommonError
    { // Ошибка переполнения.
    public:
        using CommonError::CommonError;
        std::string GetClassName() const override // Возвращает имя данного класса.
        {
            return "OverflowError";
        }
    };

    class DomainError : public CommonError
    { // Аргумент функции вне области определения.
    public:
        using CommonError::CommonError;
        std::string GetClassName() const override // Возвращает имя данного класса.
        {
            return "DomainError";
        }
    };

    class ErrorParamsInconsistency : public CommonError
    { // Несогласованность по типу и/или количеству параметров.
    public:
        using CommonError::CommonError;
        std::string GetClassName() const override
        {
            return "ErrorParamsInconsistency";
        }
    };

    class SyntaxError : public CommonError
    { // Общая синтаксическая ошибка.
    public:
        using CommonError::CommonError;
        std::string GetClassName() const override
        {
            return "SyntaxError";
        }
    };

    class ModuleError : public CommonError
    { // Ошибка при работк с внешними модулями.
    public:
        using CommonError::CommonError;
        std::string GetClassName() const override
        {
            return "ModuleError";
        }
    };

    class LogicError : public CommonError
    { // Общая логическая ошибка.
    public:
        using CommonError::CommonError;
        std::string GetClassName() const override
        {
            return "LogicError";
        }
    };

    class ReferenceError : public CommonError
    { // Ошибка при работе со ссылками.
    public:
        using CommonError::CommonError;
        std::string GetClassName() const override
        {
            return "ReferenceError";
        }
    };

    // Далее описаны классы-фабрики объектов-экземпляров описанных выше классов ошибок, предусмотренных интерпретатором.
    // Это аналоги фабричного класса NewInstance, который служит фабрикой объектов классов общего вида (программно определенных).
    // Эти же классы специализированы на изготовление экземпляров лишь одного определённого класса - какого-либо из описанных выше
    // классов ошибок. При своём исполнении (вызове функции-члена Execute()) они возвращают новый экземпляр соответствующего
    // класса, изготовлением которого они занимаются.
    using Statement = runtime::Executable;
    class NewCommonError : public Statement
    {
    public:
        NewCommonError(std::vector<std::unique_ptr<Statement>> args);
        std::pair<ThrowMessageNumber, std::string> CountParams(runtime::Closure& closure, runtime::Context& context);
        // Возвращает объект, содержащий значение типа CommonError - экземпляр класса ошибки CommonError.
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;

    protected:
        std::vector<std::unique_ptr<Statement>> args_;
    };

    class NewErrorDivisionByZero : public NewCommonError
    {
    public:
        NewErrorDivisionByZero(std::vector<std::unique_ptr<Statement>> args) :
            NewCommonError(std::move(args))
        {}
        // Возвращает объект, содержащий значение типа ErrorDivisionByZero - экземпляр класса ошибки ErrorDivisionByZero.
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    };

    class NewOverflowError : public NewCommonError
    {
    public:
        NewOverflowError(std::vector<std::unique_ptr<Statement>> args) :
            NewCommonError(std::move(args))
        {}
        // Возвращает объект, содержащий значение типа OverflowError - экземпляр класса ошибки OverflowError.
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    };

    class NewDomainError : public NewCommonError
    { // Фабричный класс для класса ошибки NewDomainError.
    public:
        NewDomainError(std::vector<std::unique_ptr<Statement>> args) :
            NewCommonError(std::move(args))
        {}
        // Возвращает объект, содержащий значение типа DomainError - экземпляр класса ошибки DomainError.
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    };

    class NewErrorParamsInconsistency : public NewCommonError
    {
    public:
        NewErrorParamsInconsistency(std::vector<std::unique_ptr<Statement>> args) :
            NewCommonError(std::move(args))
        {}
        // Возвращает объект, содержащий значение типа ErrorParamsInconsistency - экземпляр класса ошибки ErrorParamsInconsistency.
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    };

    class NewSyntaxError : public NewCommonError
    { // Фабричный класс для порождения экземпляров класса ошибки SyntaxError.
    public:
        NewSyntaxError(std::vector<std::unique_ptr<Statement>> args) :
            NewCommonError(std::move(args))
        {}
        // Возвращает объект, содержащий значение типа SyntaxError - экземпляр класса ошибки SyntaxError.
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    };

    class NewModuleError : public NewCommonError
    {
    public:
        NewModuleError(std::vector<std::unique_ptr<Statement>> args) :
            NewCommonError(std::move(args))
        {}
        // Возвращает объект, содержащий значение типа ModuleError - экземпляр класса ошибки ModuleError.
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    };

    class NewLogicError : public NewCommonError
    {
    public:
        NewLogicError(std::vector<std::unique_ptr<Statement>> args) :
            NewCommonError(std::move(args))
        {}
        // Возвращает объект, содержащий значение типа LogicError - экземпляр класса ошибки LogicError.
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    };

    class NewReferenceError : public NewCommonError
    {
    public:
        NewReferenceError(std::vector<std::unique_ptr<Statement>> args) :
            NewCommonError(std::move(args))
        {}
        // Возвращает объект, содержащий значение типа ReferenceError - экземпляр класса ошибки ReferenceError.
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    };

    [[noreturn]] void ThrowRuntimeError(runtime::Executable* exec_obj_ptr, ThrowMessageNumber msg_num, const std::string& except_text = {});
    [[noreturn]] void ThrowRuntimeError(Context& context, ThrowMessageNumber msg_num, const std::string& except_text = {});
    [[noreturn]] void RethrowRuntimeError(Context& context, TempError& orig_temp_error);
} // namespace runtime

namespace ast
{
    using Statement = runtime::Executable;
    // Подборка фабричных функций - переходников к соответствующим фабричным класса для создания объектов ошибок.
    std::unique_ptr<Statement> CreateCommonError(std::vector<std::unique_ptr<Statement>> args);
    std::unique_ptr<Statement> CreateErrorDivisionByZero(std::vector<std::unique_ptr<Statement>> args);
    std::unique_ptr<Statement> CreateOverflowError(std::vector<std::unique_ptr<Statement>> args);
    std::unique_ptr<Statement> CreateDomainError(std::vector<std::unique_ptr<Statement>> args);
    std::unique_ptr<Statement> CreateErrorParamsInconsistency(std::vector<std::unique_ptr<Statement>> args);
    std::unique_ptr<Statement> CreateSyntaxError(std::vector<std::unique_ptr<Statement>> args);
    std::unique_ptr<Statement> CreateModuleError(std::vector<std::unique_ptr<Statement>> args);
    std::unique_ptr<Statement> CreateLogicError(std::vector<std::unique_ptr<Statement>> args);
    std::unique_ptr<Statement> CreateReferenceError(std::vector<std::unique_ptr<Statement>> args);
} // namespace ast

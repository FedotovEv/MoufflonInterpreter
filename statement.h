#pragma once

#include "declares.h"
#include "runtime.h"
#include "parse.h"

#include <functional>

namespace ast
{
    struct ReturnResult
    {
        ReturnResult(runtime::ObjectHolder ret_result) : ret_result_(std::move(ret_result))
        {}
        runtime::ObjectHolder ret_result_;
    };

    enum class TerminateLoopReason
    {
        TERMINATE_LOOP_UNKNOWN = 0,
        TERMINATE_LOOP_BREAK,
        TERMINATE_LOOP_CONTINUE
    };

    struct TerminateLoop
    {
        TerminateLoop(TerminateLoopReason terminate_loop_reason) :
            terminate_loop_reason_(terminate_loop_reason)
        {}

        TerminateLoopReason terminate_loop_reason_;
    };

    using Statement = runtime::Executable;

    void MYTHLON_INTERPRETER_PUBLIC PrepareExecute(Statement* exec_obj_ptr, runtime::Context& context);

    // Выражение, возвращающее значение типа T,
    // используется как основа для создания констант
    template <typename T>
    class ValueStatement : public Statement
    {
    public:
        explicit ValueStatement(T v) : value_(std::move(v))
        {}

        runtime::ObjectHolder Execute(runtime::Closure& /*closure*/,
                                      runtime::Context& /*context*/) override
        {
            return runtime::ObjectHolder::Share(value_);
        }

    private:
        T value_;
    };

    using NumericConst = ValueStatement<runtime::Number>;
    using StringConst = ValueStatement<runtime::String>;
    using BoolConst = ValueStatement<runtime::Bool>;

    /*
    Вычисляет значение переменной либо цепочки вызовов полей объектов id1.id2.id3.
    Например, выражение circle.center.x - цепочка вызовов полей объектов в инструкции:
    x = circle.center.x
    */
    class VariableValue : public Statement
    {
    public:
        explicit VariableValue(const std::string& var_name);
        explicit VariableValue(std::vector<std::string> dotted_ids);

        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
        std::vector<std::string> GetDottedIds()
        {
            return dotted_ids_;
        }

    private:
        std::vector<std::string> dotted_ids_;
    };

    // Присваивает переменной, имя которой задано в параметре var, значение выражения rv
    class Assignment : public Statement
    {
    public:
        Assignment(std::string var, std::unique_ptr<Statement> rv);

        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    private:
        std::string var_;
        std::unique_ptr<Statement> rv_;
    };

    // Присваивает полю object.field_name значение выражения rv
    class FieldAssignment : public Statement
    {
    public:
        FieldAssignment(VariableValue object, std::string field_name, std::unique_ptr<Statement> rv);

        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    private:
        VariableValue object_;
        std::string field_name_;
        std::unique_ptr<Statement> rv_;
    };

    // Значение None
    class None : public Statement
    {
    public:
        runtime::ObjectHolder Execute([[maybe_unused]] runtime::Closure& closure,
                                      [[maybe_unused]] runtime::Context& context) override
        {
            return {};
        }
    };

    // Команда print
    class Print : public Statement
    {
    public:
        Print() = default;
        // Инициализирует команду print для вывода значения выражения argument
        explicit Print(std::unique_ptr<Statement> argument);
        // Инициализирует команду print для вывода списка значений args
        explicit Print(std::vector<std::unique_ptr<Statement>> args);

        // Инициализирует команду print для вывода значения переменной name
        static std::unique_ptr<Print> Variable(const std::string& name);

        // Во время выполнения команды print вывод должен осуществляться в поток, возвращаемый из
        // context.GetOutputStream()
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    private:
    
        std::vector<std::unique_ptr<Statement>> args_;
        std::string name_;
    };

    // Вызывает метод object.method со списком параметров args
    class MethodCall : public Statement
    {
    public:
        struct MethodCallDesc
        {
            std::unique_ptr<Statement> call_object;
            std::string call_method;
            std::vector<std::unique_ptr<Statement>> call_args;
        };

        MethodCall(std::unique_ptr<Statement> object, std::string method,
                   std::vector<std::unique_ptr<Statement>> args);

        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
        MethodCallDesc GetMethodCallDesc()
        {
            return {std::move(object_), std::move(method_), std::move(args_)};
        }
    private:

        std::unique_ptr<Statement> object_;
        std::string method_;
        std::vector<std::unique_ptr<Statement>> args_;
    };

    // Косвенное присваивание - присваивание значения некоторой переменной по вычисляемому указателю на неё.
    // Сначала вызывается метод object.method со списком параметров args.
    // Данный метод обязательно должен возвращать объект-"указатель" - runtime::PointerObject, в противном
    // случае возникнет ошибка периода исполнения - "ошибка косвенного присваивания".
    // Далее целевой переменной, на которую указывает этот объект, будет присвоено значение выражения rv.
    // Учитывая строение переменных Муфлона, можно сказать, что при этом её внутренний указатель (data_)
    // будет перенацелен на новое значение, на которое указывает результат вычисления (типа ObjectHolder)
    // выражения rv.
    class IndirectAssignment : public Statement
    {
    public:
        IndirectAssignment(std::unique_ptr<Statement> object, std::string method,
            std::vector<std::unique_ptr<Statement>> args, std::unique_ptr<Statement> rv);

        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    private:

        std::unique_ptr<Statement> object_;
        std::string method_;
        std::vector<std::unique_ptr<Statement>> args_;
        std::unique_ptr<Statement> rv_;
    };

    /*
    Создаёт новый экземпляр класса class_, передавая его конструктору набор параметров args.
    Если в классе отсутствует метод __init__ с заданным количеством аргументов,
    то экземпляр класса создаётся без вызова конструктора (поля объекта не будут проинициализированы):

    class Person:
      def set_name(name):
        self.name = name

    p = Person()
    # Поле name будет иметь значение только после вызова метода set_name
    p.set_name("Ivan")
    */
    class NewInstance : public Statement
    {
    public:
        explicit NewInstance(const runtime::Class& class_);
        NewInstance(const runtime::Class& class_, std::vector<std::unique_ptr<Statement>> args);
        // Возвращает объект, содержащий значение типа ClassInstance
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    private:
        runtime::ClassInstance new_class_instance_;
        std::vector<std::unique_ptr<Statement>> args_;
    };

    #include "special_objects_statement.h"
    #include "math_object_statement.h"

    // Базовый класс для унарных операций
    class UnaryOperation : public Statement
    {
    public:
        explicit UnaryOperation(std::unique_ptr<Statement> argument) : argument_(move(argument))
        {}

    protected:
        std::unique_ptr<Statement> argument_;
    };

    // Операция str, возвращающая строковое значение своего аргумента
    class Stringify : public UnaryOperation
    {
    public:
        using UnaryOperation::UnaryOperation;
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    };

    // Родительский класс Бинарная операция с аргументами lhs и rhs
    class BinaryOperation : public Statement
    {
    public:
        BinaryOperation(std::unique_ptr<Statement> lhs, std::unique_ptr<Statement> rhs) : lhs_(move(lhs)), rhs_(move(rhs))
        {}

    protected:
        std::unique_ptr<Statement> lhs_;
        std::unique_ptr<Statement> rhs_;
    };

    // Возвращает результат операции + над аргументами lhs и rhs
    class Add : public BinaryOperation
    {
    public:
        using BinaryOperation::BinaryOperation;

        // Поддерживается сложение:
        //  число + число
        //  строка + строка
        //  объект1 + объект2, если у объект1 - пользовательский класс с методом _add__(rhs)
        // В противном случае при вычислении выбрасывается runtime_error
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    };

    // Возвращает результат вычитания аргументов lhs и rhs
    class Sub : public BinaryOperation
    {
    public:
        using BinaryOperation::BinaryOperation;

        // Поддерживается вычитание:
        //  число - число
        // Если lhs и rhs - не числа, выбрасывается исключение runtime_error
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    };

    // Возвращает результат умножения аргументов lhs и rhs
    class Mult : public BinaryOperation
    {
    public:
        using BinaryOperation::BinaryOperation;

        // Поддерживается умножение:
        //  число * число
        // Если lhs и rhs - не числа, выбрасывается исключение runtime_error
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    };

    // Возвращает результат деления lhs и rhs
    class Div : public BinaryOperation
    {
    public:
        using BinaryOperation::BinaryOperation;

        // Поддерживается деление:
        //  число / число
        // Если lhs и rhs - не числа, выбрасывается исключение runtime_error
        // Если rhs равен 0, выбрасывается исключение runtime_error
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    };

    // Возвращает результат операции взятия остатка от деления lhs и rhs
    class ModuloDiv : public BinaryOperation
    {
    public:
        using BinaryOperation::BinaryOperation;

        // Поддерживается деление по модулю (остаток от деления):
        //  число % число
        // Если lhs и rhs - не числа, выбрасывается исключение runtime_error
        // Если rhs равен 0, выбрасывается исключение runtime_error
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    };

    // Возвращает результат вычисления логической операции or над lhs и rhs
    class Or : public BinaryOperation
    {
    public:
        using BinaryOperation::BinaryOperation;
        // Значение аргумента rhs вычисляется, только если значение lhs
        // после приведения к Bool равно False
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    };

    // Возвращает результат вычисления логической операции and над lhs и rhs
    class And : public BinaryOperation
    {
    public:
        using BinaryOperation::BinaryOperation;
        // Значение аргумента rhs вычисляется, только если значение lhs
        // после приведения к Bool равно True
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    };

    // Возвращает результат вычисления логической операции not над единственным аргументом операции
    class Not : public UnaryOperation
    {
    public:
        using UnaryOperation::UnaryOperation;
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    };

    // Составная инструкция (например: тело метода, содержимое ветки if, либо else)
    class Compound : public Statement
    {
    public:
        // Конструирует Compound из нескольких инструкций типа unique_ptr<Statement>
        template <typename... Args>
        explicit Compound(Args&&... args)
        {
            if constexpr(sizeof...(args) != 0)
                PacketAddStatement(args...);
        }

        // Добавляет очередную инструкцию в конец составной инструкции
        void AddStatement(std::unique_ptr<Statement> stmt)
        {
            comp_body_.push_back(std::move(stmt));
        }
        // Специальный метод, позволяющий получить операторы, входящие в состав сплотки.
        std::vector<const Statement*> GetCompoundStatements();
        // Последовательно выполняет добавленные инструкции. Возвращает None
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    protected:
        template <typename FirstArg, typename... Args>
        void PacketAddStatement(FirstArg&& first_arg, Args&& ... args)
        {
            comp_body_.push_back(std::move(std::forward<FirstArg>(first_arg)));
            if constexpr(sizeof...(args) != 0)
                PacketAddStatement(args...);
        }

        std::vector<std::unique_ptr<Statement>> comp_body_;
    };

    // Тело метода. Как правило, содержит составную инструкцию
    class MethodBody : public Statement
    {
    public:
        explicit MethodBody(std::unique_ptr<Statement>&& body);

        // Вычисляет инструкцию, переданную в качестве body.
        // Если внутри body была выполнена инструкция return, возвращает результат return
        // В противном случае возвращает None
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    private:
        std::unique_ptr<Statement> body_;
    };

    // Выполняет инструкцию return с выражением statement
    class Return : public Statement
    {
    public:
        explicit Return(std::unique_ptr<Statement> statement) : statement_(move(statement))
        {}

        // Останавливает выполнение текущего метода. После выполнения инструкции return метод,
        // внутри которого она была исполнена, должен вернуть результат вычисления выражения statement.
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    private:
        std::unique_ptr<Statement> statement_;
    };

    // Выполняет инструкцию return_ref с переменной dotted_ids, которая должна быть полем
    // объекта (начинаться с self).
    class ReturnRef : public Statement
    {
    public:
        explicit ReturnRef(std::vector<std::string> dotted_ids) : dotted_ids_(move(dotted_ids))
        {}

        explicit ReturnRef(std::unique_ptr<Statement> object, std::string method,
                           std::vector<std::unique_ptr<Statement>> args) :
            object_(move(object)),
            method_(move(method)),
            args_(move(args))
        {}

        // Останавливает выполнение текущего метода. После выполнения инструкции return_ref метод,
        // внутри которого она была исполнена, должен вернуть результат в виде указателя PointerObject
        // на поле dotted_ids_ текущего объекта.
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    private:
        std::vector<std::string> dotted_ids_; // Данное поле обслуживает вариант оператора return_ref
                                              // с аргументом-полем объекта (return_ref VariableValue).
        // Остальные поля служат для обработки варианта оператора return_ref с аргументом-вызовом метода
        // (return_ref MethodCall).
        std::unique_ptr<Statement> object_;
        std::string method_;
        std::vector<std::unique_ptr<Statement>> args_;

        runtime::ObjectHolder ExecuteForVariable(runtime::Closure& closure, runtime::Context& context);
        runtime::ObjectHolder ExecuteForMethod(runtime::Closure& closure, runtime::Context& context);
    };

    // Выполняет инструкцию break
    class Break : public Statement
    {
    public:
        explicit Break() = default;

        // Немедленно прекращает работу цикла while
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    };

    // Выполняет инструкцию continue
    class Continue : public Statement
    {
    public:
        explicit Continue() = default;

        // Незамедлительно переходит к началу очередной итерации цикла while
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    };

    // Объявляет класс
    class ClassDefinition : public Statement
    {
    public:
        // Гарантируется, что ObjectHolder содержит объект типа runtime::Class
        explicit ClassDefinition(runtime::ObjectHolder cls);

        // Создаёт внутри closure новый объект, совпадающий с именем класса и значением, переданным в
        // конструктор
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
        std::string GetClassName() const;
        // Функция-член GetMethodsDesc() возвращает некоторую информацию о методах класса.
        // В результирующем массиве пар каждая пара соответствует определённому методу класса.
        // Первый член пары (.first) - имя метода, второй (.second) - количество его формальных параметров.
        std::vector<std::pair<std::string, size_t>> GetMethodsDesc() const;
    private:
        runtime::ObjectHolder cls_;
    };

    // Инструкция if <condition> <if_body> else <else_body>
    class IfElse : public Statement
    {
    public:
        // Параметр else_body может быть равен nullptr
        IfElse(std::unique_ptr<Statement> condition, std::unique_ptr<Statement> if_body,
               std::unique_ptr<Statement> else_body);

        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    private:
        std::unique_ptr<Statement> condition_;
        std::unique_ptr<Statement> if_body_;
        std::unique_ptr<Statement> else_body_;
    };

    // Инструкция while <condition> <while_body>
    class While : public Statement
    {
    public:
        While(std::unique_ptr<Statement> condition, std::unique_ptr<Statement> while_body);

        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    private:
        std::unique_ptr<Statement> condition_;
        std::unique_ptr<Statement> while_body_;
    };

    // Операция сравнения
    class Comparison : public BinaryOperation
    {
    public:
        // Comparator задаёт функцию, выполняющую сравнение значений аргументов
        using Comparator = std::function<bool(const runtime::ObjectHolder&,
                                              const runtime::ObjectHolder&, runtime::Context&)>;

        Comparison(Comparator cmp, std::unique_ptr<Statement> lhs, std::unique_ptr<Statement> rhs);

        // Вычисляет значение выражений lhs и rhs и возвращает результат работы comparator,
        // приведённый к типу runtime::Bool
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    private:
        Comparator cmp_;
    };
}  // namespace ast

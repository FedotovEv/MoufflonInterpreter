#pragma once

#include "declares.h"
#include "runtime.h"
#include "parse.h"
#include "throw_messages.h" 

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
    using LeftRightStatement = runtime::LeftRightExecutable;

    // Вспомогательные элементы для обслуживания работы в составе сопрограмм.
    struct CoroCoords
    {
        // Контейнер-копия, содержащий объект-дескриптор сопрограммы (точнее, ещё один разделяемый умный указатель на
        // единственный экземпляр этого объекта), в контексте которой мы выполняемся.
        runtime::ObjectHolder coro_status_holder;
        // "Сырой" указатель на тот же объект-дескриптор нашей сопрограммы, извлечённый из контейнера.
        runtime::CoroutineInstance* coro_status_instance = nullptr;
        // Указатель на объект-хранитель состояния потока управления для данной исполняемой инструкции.
        runtime::WorkflowPosition* workflow_current = nullptr;
        // Признак того, что сейчас сопрограмма находится в процессе возобновления своего выполнения
        // (движется к точке своей последней приостановки).
        bool is_resume_execution_now = false;
    };
    CoroCoords GetCoYieldCoroCoords(runtime::Closure& closure, Statement* this_statement);

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

    // Присваивает переменной, имя которой задано в параметре var, значение выражения rv.
    class Assignment : public LeftRightStatement
    {
    public:
        Assignment(std::string var, std::unique_ptr<Statement> rv);

        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
        // Методы раздельного исполнения левой и правой части оператора присваивания.
        runtime::ObjectHolder ExecuteRight(runtime::Closure& closure, runtime::Context& context) override;
        runtime::ObjectHolder ExecuteLeft
            (runtime::ObjectHolder&& right_result, runtime::Closure& closure, runtime::Context& context) override;

    private:
        std::string var_;
        std::unique_ptr<Statement> rv_;
    };

    // Присваивает полю object.field_name значение выражения rv.
    class FieldAssignment : public LeftRightStatement
    {
    public:
        FieldAssignment(VariableValue object, std::string field_name, std::unique_ptr<Statement> rv);

        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
        // Методы раздельного исполнения левой и правой части оператора присваивания.
        runtime::ObjectHolder ExecuteRight(runtime::Closure& closure, runtime::Context& context) override;
        // Метод исполнения левой части оператора присваивания полю объекта (состоит из вычисления целевого объекта, и,
        // собственно, самого присваивания).
        runtime::ObjectHolder ExecuteLeft
            (runtime::ObjectHolder&& right_result, runtime::Closure& closure, runtime::Context& context) override;

    private:
        VariableValue object_;
        std::string field_name_;
        std::unique_ptr<Statement> rv_;
    };

    // Инструкция удаление простой переменной из таблицы символов.
    class DeleteVariable : public Statement
    {
    public:
        DeleteVariable(std::string var);
        // Удаляет переменную var_ из таблицы символов closure.
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;

    private:
        std::string var_;
    };

    // Инструкция удаление поля объекта из его таблицы символов.
    class DeleteField : public Statement
    {
    public:
        DeleteField(VariableValue object, std::string field_name);
        // Удаляет поле field_name_ из таблицы символов объекта object_, находящейся внутри таблицы closure.
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;

    private:
        VariableValue object_;
        std::string field_name_;
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
        // Инициализирует команду print для вывода значения выражения argument.
        explicit Print(std::unique_ptr<Statement> argument);
        // Инициализирует команду print для вывода списка значений args.
        explicit Print(std::vector<std::unique_ptr<Statement>> args);

        // Инициализирует команду print для вывода значения переменной name
        static std::unique_ptr<Print> Variable(const std::string& name);

        // Во время выполнения команды print вывод должен осуществляться в поток, возвращаемый из
        // context.GetOutputStream().
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
            std::string parent_name;
            bool is_dereference_result = true;
        };

        MethodCall(std::unique_ptr<Statement> object, std::string method,
                   std::vector<std::unique_ptr<Statement>> args, std::string parent_name = {});

        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;

        // Функция извлечения и переноса внутреннего содержимого объекта MethodCall в промежуточное хранилище
        // типа MethodCallDesc. Сам объект MethodCall остаётся после этого в неопределённом состоянии.
        MethodCallDesc MoveMethodCallToDesc()
        {
            return {.call_object = std::move(object_), .call_method = std::move(method_), .call_args = std::move(args_),
                    .parent_name = std::move(parent_name_), .is_dereference_result = is_dereference_result_};
        }

        ast::VariableValue* GetCallObject()
        {
            return dynamic_cast<ast::VariableValue*>(object_.get());
        }

        void SetDereferenceFlag(bool is_dereference_result)
        {
            is_dereference_result_ = is_dereference_result;
        }

    private:
        std::unique_ptr<Statement> object_;
        std::string method_;
        std::vector<std::unique_ptr<Statement>> args_;
        std::string parent_name_;
        // Признак автоматического разыменования указателя, если он является результатом исполнения данного метода.
        bool is_dereference_result_ = true;
    };

    // Косвенное присваивание - присваивание значения некоторой переменной по вычисляемому указателю на неё.
    // Сначала вызывается метод left_method_call_ (обязательно представляет собой объект класса ast::MethodCall).
    // Данный метод обязательно должен возвращать объект-"указатель" - runtime::PointerObject, в противном
    // случае возникнет ошибка периода исполнения - "ошибка косвенного присваивания".
    // Далее целевой переменной, на которую указывает этот объект, будет присвоено значение выражения rv.
    // Учитывая строение переменных Муфлона, можно сказать, что при этом её внутренний указатель (data_)
    // будет перенацелен на новое значение, на которое указывает результат вычисления (типа ObjectHolder)
    // выражения rv.
    class IndirectAssignment : public LeftRightStatement
    {
        friend class CoAwait;

    public:
        IndirectAssignment
            (std::unique_ptr<Statement> left_method_call, std::unique_ptr<Statement> rv, std::string parent_name = {});

        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
        // Методы раздельного исполнения левой и правой части оператора косвенного присваивания.
        runtime::ObjectHolder ExecuteRight(runtime::Closure& closure, runtime::Context& context) override;
        // Функция-член исполнения "левой" части присваивания - вычисление ссылки на целевую переменную и присваивание ей
        // значения right_result.
        runtime::ObjectHolder ExecuteLeft
            (runtime::ObjectHolder&& right_result, runtime::Closure& closure, runtime::Context& context) override;

    private:
        std::unique_ptr<Statement> left_method_call_;
        std::unique_ptr<Statement> rv_;
        std::string parent_name_;
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
        explicit NewInstance(const runtime::Class& class_, std::vector<std::unique_ptr<Statement>> args);
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

    // Операция str, возвращающая строковое значение своего аргумента.
    class Stringify : public UnaryOperation
    {
    public:
        using UnaryOperation::UnaryOperation;
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    };

    // Родительский класс Бинарная операция с аргументами lhs и rhs.
    class BinaryOperation : public Statement
    {
    public:
        BinaryOperation(std::unique_ptr<Statement> lhs, std::unique_ptr<Statement> rhs) : lhs_(move(lhs)), rhs_(move(rhs))
        {}

    protected:
        std::unique_ptr<Statement> lhs_;
        std::unique_ptr<Statement> rhs_;
    };

    // Операция is_same_target, возвращающая "ИСТИНУ", если оба её аргумента указывают на одно и то же место в памяти
    // (на один и тот же объект).
    class IsSameTarget : public BinaryOperation
    {
    public:
        using BinaryOperation::BinaryOperation;
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    };

    // Возвращает результат операции + над аргументами lhs и rhs
    class Add : public BinaryOperation
    {
    public:
        using BinaryOperation::BinaryOperation;

        // Поддерживается сложение:
        //  число + число
        //  строка + строка
        //  объект1 + объект2, если у объект1 - пользовательский класс с методом __add__(rhs)
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

    // Возвращает результат побитовой инверсии единственного аргумента
    class Complement : public UnaryOperation
    {
    public:
        using UnaryOperation::UnaryOperation;

        // Поддерживается выполнение побитового отрицания (двоичное дополнение, комплемент, ~).
        // Для всех чисел (как целых, так и дробных) выполняется прямое ~.
        // В случае числа с плавающей точкой внутренняя структура такого числа игнорируется.
        // Строки инвертируются посимвольно, действие выполняетя для всех содержащихся в них символов.
        // Результат - строка такой же длины, каждый символ которой двоично обращён.
        // Для всех остальных типов битовое отрицание эквивалентно логическому (not).
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    };

    // Возвращает результат вычисления побитовой операции | (или) над lhs и rhs
    class BitwiseOr : public BinaryOperation
    {
    public:
        using BinaryOperation::BinaryOperation;
        // Расчёт побитового ИЛИ идёт по четырём различным путям:
        // 1. Оба аргумента целые числа. Результат - целое число, являющееся результатом применения к ним
        //    побитового ИЛИ.
        // 2. Оба аргумента дробные числа. Вычисление выполняется аналогично целым числам, игнорируя
        //    внутреннюю структуру дробного числа (они представляются как простые восьмибайтовые поля).
        //    Результат также возвращается как дробное число.
        // 3. Оба операнда - строки. Результат - строка, каждый символ которой есть результат побитового ИЛИ
        //    над соответствующими по порядку символами аргументов. Более короткая строка дополняется при этом
        //    нулевыми символами справа.
        // 4. Для всех прочих случаев побитовое ИЛИ эквивалентно логическому ИЛИ. Результат - True или False.
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    };

    // Возвращает результат вычисления побитовой операции & (и) над lhs и rhs
    class BitwiseAnd : public BinaryOperation
    {
        using BinaryOperation::BinaryOperation;
        // Расчёт побитового И идёт по четырём различным путям:
        // 1. Оба аргумента целые числа. Результат - целое число, являющееся результатом применения к ним
        //    побитового И.
        // 2. Оба аргумента дробные числа. Вычисление выполняется аналогично целым числам, игнорируя
        //    внутреннюю структуру дробного числа (они представляются как простые восьмибайтовые поля).
        //    Результат также возвращается как дробное число.
        // 3. Оба операнда - строки. Результат - строка, каждый символ которой есть результат побитового И
        //    над соответствующими по порядку символами аргументов. Более короткая строка дополняется при этом
        //    нулевыми символами справа.
        // 4. Для всех прочих случаев побитовое И эквивалентно логическому И. Результат - True или False.
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    };

    // Возвращает результат вычисления побитовой операции ^ (исключающее или) над lhs и rhs
    class BitwiseXor : public BinaryOperation
    {
        using BinaryOperation::BinaryOperation;
        // Расчёт побитового XOR идёт по четырём различным путям:
        // 1. Оба аргумента целые числа. Результат - целое число, являющееся результатом применения к ним
        //    побитового XOR.
        // 2. Оба аргумента дробные числа. Вычисление выполняется аналогично целым числам, игнорируя
        //    внутреннюю структуру дробного числа (они представляются как простые восьмибайтовые поля).
        //    Результат также возвращается как дробное число.
        // 3. Оба операнда - строки. Результат - строка, каждый символ которой есть результат побитового XOR
        //    над соответствующими по порядку символами аргументов. Более короткая строка дополняется при этом
        //    нулевыми символами справа.
        // 4. Для всех прочих случаев побитовое ИЛИ эквивалентно логическому XOR. Результат - True или False.
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    };

    // Возвращает результат вычисления побитового сдвига влево << над lhs и rhs
    class ShiftLeft : public BinaryOperation
    {
        using BinaryOperation::BinaryOperation;
        // Правый аргумент rhs указывает величину сдвига. Левый аргумент lhs - это то, к чему сдвиг применяется.
        // То есть rhs - это количество битов, на которое будет выполнен сдвиг влево левого аргумента lhs.
        // Далее расчёт может идти по двум путям в зависимости от типа операндов:
        // 1. Левый и правый аргументы - числа. В этом случае результат рассчитывается как простой сдвиг влево
        //    левого аргумента на количество бит, определённое значение правого аргумента. Если левый аргумент
        //    дробное число, то операция выполняется непосредственно над его двоичным представлением, игнорируя
        //    его внутреннюю структуру. Правый аргумент всегда воспринимается как целое число, если нужно -
        //    округляется.
        // 2. Оба операнда - строки. Результат - также строка, равная по длине левому аргументу lhs. Каждый её
        //    символ есть результат побитового сдвига влево кода каждого символа исходной левой строки lhs на
        //    количество бит, определяемое кодом соответствующим по порядку символа правой строки-аргумента rhs.
        //    Лишние символы rhs игнорируются, недостающие дополняются нулями.
        // 3. Для всех прочих случаев будет выброшено исключение с ошибкой периода исполнения.
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    };

    // Возвращает результат вычисления побитового сдвига вправо >> над lhs и rhs
    class ShiftRight : public BinaryOperation
    {
        using BinaryOperation::BinaryOperation;
        // Правый аргумент rhs указывает величину сдвига. Левый аргумент lhs - это то, к чему сдвиг применяется.
        // То есть rhs - это количество битов, на которое будет выполнен сдвиг вправо левого аргумента lhs.
        // Далее расчёт может идти по двум путям в зависимости от типа операндов:
        // 1. Левый и правый аргументы - числа. В этом случае результат рассчитывается как простой сдвиг вправо
        //    левого аргумента на количество бит, определённое значение правого аргумента. Если левый аргумент
        //    дробное число, то операция выполняется непосредственно над его двоичным представлением, игнорируя
        //    его внутреннюю структуру. Правый аргумент всегда воспринимается как целое число, если нужно -
        //    округляется.
        // 2. Оба операнда - строки. Результат - также строка, равная по длине левому аргументу lhs. Каждый её
        //    символ есть результат побитового сдвига вправо кода каждого символа исходной левой строки lhs на
        //    количество бит, определяемое кодом соответствующим по порядку символа правой строки-аргумента rhs.
        //    Лишние символы rhs игнорируются, недостающие дополняются нулями.
        // 3. Для всех прочих случаев будет выброшено исключение с ошибкой периода исполнения.
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

    // Возвращает результат вычисления логической операции xor над lhs и rhs
    class Xor : public BinaryOperation
    {
    public:
        using BinaryOperation::BinaryOperation;
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

        runtime::ProgramCommandDescriptor GetLastCommandDesc()
        { // Возвращает дескриптор последней команды сплотки        
            return last_body_command_desc_;
        }

        // Добавляет очередную инструкцию в конец составной инструкции
        void AddStatement(std::unique_ptr<Statement> stmt);

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
        runtime::ProgramCommandDescriptor last_body_command_desc_; // дескриптор последней команды сплотки
    };

    /*
    // "Головная" сплотка - составная инструкция, представляющая тело самой программы как таковое (находится
    // на самом верхнем уровне её блочной структуры). В каждой программе такая сплотка всегда есть и всегда только одна,
    // она создаётся в методе ParseProgram() синтаксического анализатора Parser. По сравнению с обычной составной
    // инструкцией класса Compound в структуре ProgramCompound хранится некоторая дополнительная метаинформация,
    // характеризующая всю программу в целом и необходимая для работы некоторым прочим её узлам.
    class ProgramCompound : public Compound
    {
    public:
        explicit ProgramCompound() = default;

        std::unordered_map<std::string, ast::PluginDescData>& GetPlugines()
        {
            return plugines_;
        }

    private:
        // Описание втыкал, подключённых к программе директивами import в процессе её синтаксического анализа.
        std::unordered_map<std::string, ast::PluginDescData> plugines_;
    };
    */

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
        std::unique_ptr<runtime::PsevdoExecutable> dummy_statement_ = std::make_unique<runtime::PsevdoExecutable>();
    };

    // Объект исполнения инструкции выброса исключения raise с выражением (аргументом) statement.
    class Raise : public Statement
    {
    public:
        explicit Raise(std::unique_ptr<Statement> statement) : statement_(move(statement))
        {
            SetCommandGenus(runtime::CommandGenus::CMD_GENUS_RETURN_FROM_METHOD);
        }

        // Выбрасывает исключение RuntimeError с объектом ошибки, возвращаемом при вычислении выражения statement_.
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    
    private:
        std::unique_ptr<Statement> statement_;
    };

    // Выполняет инструкцию return с выражением statement
    class Return : public Statement
    {
    public:
        explicit Return(std::unique_ptr<Statement> statement) : statement_(move(statement))
        {
            SetCommandGenus(runtime::CommandGenus::CMD_GENUS_RETURN_FROM_METHOD);
        }

        // Останавливает выполнение текущего метода. После выполнения инструкции return метод,
        // внутри которого она была исполнена, должен вернуть результат вычисления выражения statement.
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    
    private:
        std::unique_ptr<Statement> statement_;
    };

    // Выполняет инструкцию co_yield с выражением statement.
    class CoYield : public Statement
    {
    public:
        explicit CoYield(std::unique_ptr<Statement> statement) : statement_(move(statement))
        {
            SetCommandGenus(runtime::CommandGenus::CMD_GENUS_RETURN_FROM_METHOD);
        }

        // Приостанавливает выполнение текущей сопрограммы, сохраняя снимок её мгновенного состояния в её объект-дескриптор.
        // После выполнения инструкции co_yield метод-сопрограмма, внутри которого она была исполнена, сохраняет
        // промежуточный (очередной) результат своей деятельности в дескриптор сопрограммы, а также возвращает его как свое
        // возвращаемое значение.
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    
    private:
        std::unique_ptr<Statement> statement_;
    };

    // Исполнитель инструкции co_await над оператором присваивания statement.
    class CoAwait : public Statement
    {
    public:
        explicit CoAwait(std::unique_ptr<Statement> statement);    
        // Оператор statement_ полагается оператором присваивания (Assignment, FieldAssignment, IndirectAssignment либо MethodCall -
        // - один из тех исполняемых объектов, которые могут быть возвращены синтаксическим анализатором ParseAssignmentOrCall()).
        // Инструкция анализирует правую часть такого оператора.
        // 1. Если результат её исполнения приводится к какому-либо объекту-ждуну(наследнику Awaitbale), то запрашиваются его
        // специальные методы, по результатам работы которых производится (либо, напротив, не производится) приостановка сопрограммы,
        // а затем её возможное возобновление с присваиванием определённого значения левой части оператора.
        // 2. Если правая часть оператора после исполнения даёт объект класса CoroutineInstance, то ждун запрашивается у этого
        // объекта, а далее работа производится по первому варианту данного списка.
        // 3. Во всех прочих случаях оператор присваивания выполняется обычным способом, а наличие у него префикса co_await не имеет
        // значения и фактически игнорируется.
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;

    private:
        enum class StatementType
        { // Тип оператора statement_, который был нам передан как аргумент инструкции co_wait.
            STATEMENT_NONE = 0,
            STATEMENT_SIMPLE_ASSIGN,
            STATEMENT_FIELD_ASSIGN,
            STATEMENT_INDIRECT_ASSIGN,
            STATEMENT_OTHER
        };

        std::unique_ptr<Statement> statement_;
        // Характеристики оператора-параметра statement_.
        StatementType statement_type_ = StatementType::STATEMENT_NONE;
        ast::Assignment* assign_stat_ = nullptr;                    // Указатель на исполняемый объект типа "простое присваивание", если statement_ является именно им.
        ast::FieldAssignment* field_assign_stat_ = nullptr;         // Указатель на исполняемый объект типа "присваивание полю объекта", если statement_ является им.
        ast::IndirectAssignment* indirect_assign_stat_ = nullptr;   // Указатель на исполняемый объект типа "косвенное присваивание", если statement_ является именно им.
        ast::LeftRightStatement* left_right_stat_ = nullptr;        // Указатель на двухтактный исполняемый объект, если statement_ относится к таковым.
    };

    // Выполняет инструкцию return_ref dotted_ids, возвращая ссылку на переменную dotted_ids, которая должна быть полем
    // объекта (начинаться с self), или ретранслирует вызывающей команде результат работы другого метода, возвращающего
    // такую сслыку (вариант return_ref method_call_id).
    // Этот же класс также выполняет аналогичные функции инструкции co_yield_ref, выполняющейся в составе сопрограммы и,
    // кроме возврата ссылки, также формирует точку её приостановки и последующего возобновления.
    class ReturnRef : public Statement
    {
    public:
        // Вариант конструктора, если аргументом return_ref выступает переменная.
        explicit ReturnRef(std::vector<std::string> dotted_ids, bool is_co_yield_ref = false) :
            dotted_ids_(move(dotted_ids)), is_co_yield_ref_(is_co_yield_ref)
        {
            SetCommandGenus(runtime::CommandGenus::CMD_GENUS_RETURN_FROM_METHOD);
        }

        // Вариант конструктора, если аргументом return_ref является вызов метода (который,
        // в свою очередь, должен возвратить указатель PointerObject).
        explicit ReturnRef(std::unique_ptr<Statement> argument_statement, bool is_co_yield_ref = false) :
            argument_statement_(move(argument_statement)), is_co_yield_ref_(is_co_yield_ref)
        {
            SetCommandGenus(runtime::CommandGenus::CMD_GENUS_RETURN_FROM_METHOD);
            // Так как от метода здесь ожидается именно указатель (ссылка) на некоторое поле данного объекта, то
            // автоматическое разыменование такой ссылки методом нужно отключить.
            if (MethodCall* arg_method_ptr = dynamic_cast<MethodCall*>(argument_statement_.get()))
                arg_method_ptr->SetDereferenceFlag(false);
        }

        // Останавливает выполнение текущего метода. После выполнения инструкции return_ref метод,
        // внутри которого она была исполнена, возвращает результат в виде указателя PointerObject.
        // Он может либо указывать на поле dotted_ids_ объекта-аргумента, либо быть ретрансляцией
        // аналогичного указателя, возвращённого методом-аргументом method_.
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;

    private:
        std::vector<std::string> dotted_ids_; // Данное поле обслуживает вариант оператора return_ref
                                              // с аргументом-полем объекта (return_ref VariableValue).
        std::unique_ptr<Statement> argument_statement_; // Поле служит для обслуживания варианта оператора с вызовом
                                                        // метода в качестве аргумента (return_ref MethodCall).
        // Признак, указывающий на то, что объект является исполнителем инструкции co_yield_ref.
        bool is_co_yield_ref_ = false;

        runtime::ObjectHolder ExecuteForVariable(runtime::Closure& closure, runtime::Context& context);
        runtime::ObjectHolder ExecuteForMethod(runtime::Closure& closure, runtime::Context& context);
    };

    // Выполняет инструкцию break.
    class Break : public Statement
    {
    public:
        explicit Break() = default;

        // Немедленно прекращает работу цикла while.
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    };

    // Выполняет инструкцию continue.
    class Continue : public Statement
    {
    public:
        explicit Continue() = default;

        // Незамедлительно переходит к началу очередной итерации цикла while
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    };

    // "Холостая" (нефункциональная) инструкция pass.
    class Pass : public Statement
    {
    public:
        explicit Pass() = default;

        // Холостой оператор-заполнитель - не делает ничего содержательного.
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    };

    // Объявляет класс. Явлется определителем класса, образующим особый исполнимый узел АСД, соответствующий точке его
    // определения по ходу исполнения программы. После его исполнения данный класс заносится в таблицу символов и становится
    // доступным для создания его экземпляров runtime::ClassInstance с помощью фабричного узла АСД ast::NewInstance.
    class ClassDefinition : public Statement
    {
        friend class runtime::TypeTraitsInstance;

    public:
        // Гарантируется, что ObjectHolder содержит объект типа runtime::Class.
        explicit ClassDefinition(runtime::ObjectHolder cls);

        // Создаёт внутри closure новый объект, совпадающий с именем класса и значением, переданным в
        // конструктор.
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
        std::string GetClassName() const;
        // Функция-член GetMethodsDesc() возвращает некоторую информацию о методах класса.
        // В результирующем массиве пар каждая пара соответствует определённому методу класса.
        // Первый член пары (.first) - имя метода, второй (.second) - количество его формальных параметров.
        std::vector<std::pair<std::string, size_t>> GetMethodsDesc() const;
    
    private:
        runtime::ObjectHolder cls_;

        // Функция возвращает указатель на класс (runtime::Class), хранящийся в cls_.
        runtime::Class* GetClass();
    };

    // Инструкция if <condition> <if_body> else <else_body>.
    class IfElse : public Statement
    {
    public:
        // Параметр else_body может быть равен nullptr.
        IfElse(std::unique_ptr<Statement> condition, std::unique_ptr<Statement> if_body,
               std::unique_ptr<Statement> else_body);

        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    private:
        std::unique_ptr<Statement> condition_;
        std::unique_ptr<Statement> if_body_;
        std::unique_ptr<Statement> else_body_;
    };

    // Инструкция while <condition> <while_body>.
    class While : public Statement
    {
    public:
        While(std::unique_ptr<Statement> condition, std::unique_ptr<Statement> while_body);

        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    private:
        std::unique_ptr<Statement> condition_;
        std::unique_ptr<Statement> while_body_;
    };

    // Блок обработки исключений. Структура инструкции состоит из контролируемого блока try <try_body>, последовательности
    // селективных перехватчиков except <class_name> as <var_name> <except_body>, неселективного перехватичка вида except <except_body>,
    // блока обработки нормального завершения else <else_body> и блока общего послелога finally <finally_body>.
    // try <try_body> {except <class_name> as <var_name> <except_body> ...} {except <except_body>} {else <else_body>} {finally <finally_body>}.
    class TryExcept : public Statement
    {
    public:
        // Классы ExceptClassVarPair и ExceptBlockDesc определяют структуру дескриптора отдельного except-терма блочной инструкции
        // try..{except ...} ... .
        struct ExceptClassVarPair
        {
            std::string class_name;
            std::string var_name;
        };

        using ClassVarPairList = std::vector<ExceptClassVarPair>;

        struct ExceptBlockDesc
        {
            ClassVarPairList class_var_pairs;
            std::unique_ptr<Statement> except_body;
        };

        using ExceptBlockList = std::vector<ExceptBlockDesc>;

        // Тела термов else_body и finally_body могут быть равными nullptr, массив except_blocks также может оказаться пуст.
        TryExcept(std::unique_ptr<Statement> try_body, ExceptBlockList except_blocks,
                  std::unique_ptr<Statement> else_body, std::unique_ptr<Statement> finally_body);
        // Безусловно исполняет защищаемый блок try_body, перехватывает возникшие при этом исключения, анализирует их на
        // принадлежность одному из except-термов, при обнаружении первого сходства исполняет его тело. При отсутствии
        // исключений, порождённых ошибками периода исполнения внутри контролируемого блока, исполняется содержимое
        // блока else_body. Наконец, завершающим шагом безусловно и всегда исполняется блок finally_body.
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
    private:
        std::unique_ptr<Statement> try_body_;
        ExceptBlockList except_blocks_;
        std::unique_ptr<Statement> else_body_;
        std::unique_ptr<Statement> finally_body_;
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

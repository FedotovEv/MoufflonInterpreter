#pragma once

#include "declares.h"
#include "throw_messages.h"
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <map>
#include <variant>
#include <atomic>

namespace ast
{
    class CoYield;
}

namespace runtime
{
    enum MethodParamCheckMode
    {
        PARAM_CHECK_NONE = 0,
        PARAM_CHECK_QUANTITY_EQUAL = 1,
        PARAM_CHECK_QUANTITY_LESS_EQ = 2,
        PARAM_CHECK_QUANTITY_GREATER_EQ = 3,
        PARAM_CHECK_TYPE = 4,
        PARAM_CHECK_TYPE_QUANTITY_EQUAL = 5,
        PARAM_CHECK_TYPE_QUANTITY_LESS_EQ = 6,
        PARAM_CHECK_TYPE_QUANTITY_GREATER_EQ = 7
    };

    enum MethodParamType
    {
        PARAM_TYPE_ANY = 0,
        PARAM_TYPE_NUMERIC = 1,
        PARAM_TYPE_STRING = 2,
        PARAM_TYPE_LOGICAL = 4,
        PARAM_TYPE_NONE = 8,
        PARAM_TYPE_NUMERIC_STRING = 3,
        PARAM_TYPE_NUMERIC_LOGICAL = 5,
        PARAM_TYPE_STRING_LOGICAL = 6,
        PARAM_TYPE_NUMERIC_NONE = 9,
        PARAM_TYPE_STRING_NONE = 10,
        PARAM_TYPE_LOGICAL_NONE = 12,
        PARAM_TYPE_NUMERIC_STRING_NONE = 11,
        PARAM_TYPE_NUMERIC_LOGICAL_NONE = 13,
        PARAM_TYPE_STRING_LOGICAL_NONE = 14,
        PARAM_TYPE_NUMERIC_STRING_LOGICAL = 7,
        PARAM_TYPE_NUMERIC_STRING_LOGICAL_NONE = 15
    };
    
    enum class CommandGenus
    {
        CMD_GENUS_UNKNOWN = 0,
        CMD_GENUS_CALL_METHOD,              // Это команда вызова метода класса
        CMD_GENUS_RETURN_FROM_METHOD,
        CMD_GENUS_AFTER_LAST_METHOD_STMT,
        CMD_GENUS_INITIALIZE
    };

    // Контекст исполнения инструкций Mython
    class MYTHLON_INTERPRETER_PUBLIC Context
    {
    public:
        // Возвращает поток вывода для команд print
        virtual std::ostream& GetOutputStream() = 0;
        virtual LinkageFunction& GetExternalLinkage() = 0;
        virtual bool IsTerminated() = 0;
        virtual void SetTerminate() = 0;
        virtual void Clear() = 0;

        ProgramCommandDescriptor GetLastCommandDesc()
        {
            return last_command_desc_;
        }

        void SetLastCommandDesc(ProgramCommandDescriptor last_command_desc)
        {
            last_command_desc_ = last_command_desc;
        }

    protected:
        ~Context() = default;

    private:
        ProgramCommandDescriptor last_command_desc_;
    };

    // Базовый класс для всех объектов языка Mython
    class MYTHLON_INTERPRETER_PUBLIC Object
    {
    public:
        virtual ~Object() = default;
        virtual size_t SizeOf() const = 0;
        virtual const void* GetPtr() const = 0;
        // выводит в os своё представление в виде строки
        virtual void Print(std::ostream& os, Context& context) = 0;
    };

    class CommonClassInstance;
    // Специальный класс-обёртка, предназначенный для хранения объекта в Mython-программе
    class MYTHLON_INTERPRETER_PUBLIC ObjectHolder
    {
    public:
        // Создаёт пустое значение
        ObjectHolder() = default;

        // Возвращает ObjectHolder, владеющий объектом типа T
        // Тип T - конкретный класс-наследник Object.
        // object копируется или перемещается в кучу
        template <typename T>
        [[nodiscard]] static ObjectHolder Own(T&& object)
        {
            return ObjectHolder(std::make_shared<T>(std::forward<T>(object)));
        }

        // Создаёт ObjectHolder, не владеющий объектом (аналог слабой ссылки)
        [[nodiscard]] static ObjectHolder Share(Object& object);
        // Создаёт пустой ObjectHolder, соответствующий значению None
        [[nodiscard]] static ObjectHolder None();

        // Возвращает ссылку на Object внутри ObjectHolder.
        // ObjectHolder должен быть непустым
        Object& operator*() const;

        Object* operator->() const;

        [[nodiscard]] Object* Get() const;

        // Возвращает указатель на объект типа T либо nullptr, если внутри ObjectHolder не хранится
        // объект данного типа
        template <typename T>
        [[nodiscard]] T* TryAs() const
        {
            return dynamic_cast<T*>(this->Get());
        }

        // Возвращает true, если ObjectHolder не пуст.
        explicit operator bool() const;

        // Возвращает "ИСТИНУ", если указатель владеющий.
        bool IsOwning() const;
        
        // Модифицирует содержимое объекта, перенацеливая указатель data_ на тот объект,
        // на который указывает data_ внутри аргумента object_holder.
        void ModifyData(const ObjectHolder& object_holder);
        
    private:
        explicit ObjectHolder(std::shared_ptr<Object> data);
        bool IsOwning(const std::shared_ptr<Object>& test_ptr) const;
        void AssertIsValid() const;

        // Пустой удалитель, применяемый для невладеющих вместилищ, возвращаемых методом Share().
        static void empty_deleter(Object*)
        {}  // Не делает ничего. Абсолютно ничего.

        std::shared_ptr<Object> data_;
    };

    // Объект-значение, хранящий значение типа T.
    template <typename T>
    class MYTHLON_INTERPRETER_PUBLIC ValueObject : public Object
    {
    public:
        ValueObject(T v) : value_(v)      
        {}

        void Print(std::ostream& os, [[maybe_unused]] Context& context) override
        {
            os << value_;
        }

        [[nodiscard]] const T& GetValue() const
        {
            return value_;
        }

        const void* GetPtr() const
        {
            return &value_;
        }
        
        size_t SizeOf() const
        {
            return sizeof(value_);
        }

    private:
        T value_;
    };

    class MYTHLON_INTERPRETER_PUBLIC PointerObject : public Object
    {
    public:
        PointerObject() : object_ptr_(nullptr)
        {}

        PointerObject(ObjectHolder* object_ptr) : object_ptr_(object_ptr)
        {}

        void Print(std::ostream& os, [[maybe_unused]] Context& context) override
        {
            os << object_ptr_;
        }

        [[nodiscard]] ObjectHolder* GetPointer() const
        {
            return object_ptr_;
        }

        const void* GetPtr() const
        {
            return reinterpret_cast<const void*>(&object_ptr_);
        }

        size_t SizeOf() const
        {
            return sizeof(object_ptr_);
        }

    private:
        ObjectHolder* object_ptr_;
    };

    // Таблица символов, связывающая имя объекта с его значением
    using Closure = std::unordered_map<std::string, ObjectHolder>;

    struct CallStackEntry
    {
        ProgramCommandDescriptor call_command; // Строка исходника, в которой находится
                                               // точка вызова метода, создавшего данный стековый кадр.
        ProgramCommandDescriptor first_command; // Строка первой исполняемой команды данного стекового кадра
        Closure* closure_ptr = nullptr; // Указатель на таблицу символов стекового кадра (таблицу символов
                                        // его головного метода).
        std::string info_data; // Имя стекового кадра (например, имя метода, которому этот кадр принадлежит)
    };

    // Проверяет, содержится ли в object значение, приводимое к True
    // Для отличных от нуля чисел, True и непустых строк возвращается true. В остальных случаях - false.
    bool IsTrue(const ObjectHolder& object);

    // Интерфейс для выполнения действий над объектами Mython
    class MYTHLON_INTERPRETER_PUBLIC Executable
    {
    public:
        Executable() = default;
        virtual ~Executable() = default;
        // Выполняет действие над объектами внутри closure, используя context
        // Возвращает результирующее значение либо None
        virtual ObjectHolder Execute(Closure& closure, Context& context) = 0;
        ProgramCommandDescriptor GetCommandDesc()
        {
            return command_desc_;
        }

        void SetCommandDesc(ProgramCommandDescriptor command_desc)
        {
            command_desc_ = command_desc;
        }

        CommandGenus GetCommandGenus()
        {
            return command_genus_;
        }

        void SetCommandGenus(CommandGenus command_genus)
        {
            command_genus_ = command_genus;
        }

    private:
        CommandGenus command_genus_ = CommandGenus::CMD_GENUS_UNKNOWN;
        ProgramCommandDescriptor command_desc_;
    };

    // Строковое значение
    class String : public ValueObject<std::string>
    {
    public:
        using ValueObject<std::string>::ValueObject;

        const void* GetPtr() const
        {
            return GetValue().data();
        }

        size_t SizeOf() const
        {
            return GetValue().size();
        }
    };

    // Далее описываются структуры, необходимые для работы с числовыми значениями
    class MYTHLON_INTERPRETER_PUBLIC Number : public Object
    {
    public:
        Number(NumberValue v) : value_(v)
        {}

        Number(int v) : value_(v)
        {}

        Number(double v) : value_(v)
        {}

        Number(const Number&) = default;
        Number(Number&&) = default;

        void Print(std::ostream& os, [[maybe_unused]] Context& context) override;

        bool IsInt() const noexcept
        {
            return std::holds_alternative<int>(value_);
        }

        bool IsDouble() const noexcept
        {
            return std::holds_alternative<double>(value_);
        }

        [[nodiscard]] const NumberValue& GetValue() const
        {
            return value_;
        }

        [[nodiscard]] int GetIntValue() const
        {
            return ImplGetValue<int>();
        }

        [[nodiscard]] double GetDoubleValue() const
        {
            return ImplGetValue<double>();
        }

        const void* GetPtr() const;
        size_t SizeOf() const;

    private:
        template <typename T>
        T ImplGetValue() const
        {
            if (std::holds_alternative<int>(value_))
                return static_cast<T>(std::get<int>(value_));
            else if (std::holds_alternative<double>(value_))
                return static_cast<T>(std::get<double>(value_));
            else
                return T(0);
        }

        NumberValue value_;
    };

    Number operator+(const Number& first_op, const Number& second_op);
    Number operator-(const Number& first_op, const Number& second_op);
    Number operator*(const Number& first_op, const Number& second_op);
    Number operator/(const Number& first_op, const Number& second_op);
    Number operator%(const Number& first_op, const Number& second_op);
    Number operator|(const Number& first_op, const Number& second_op);
    Number operator&(const Number& first_op, const Number& second_op);
    Number operator^(const Number& first_op, const Number& second_op);
    Number operator<<(const Number& first_op, const Number& second_op);
    Number operator>>(const Number& first_op, const Number& second_op);
    Number operator~(const Number& first_op);
    bool operator<(const Number& first_op, const Number& second_op);
    bool operator==(const Number& first_op, const Number& second_op);

    // Логическое значение
    class MYTHLON_INTERPRETER_PUBLIC Bool : public ValueObject<bool>
    {
    public:
        using ValueObject<bool>::ValueObject;

        void Print(std::ostream& os, Context& context) override;
    };

    // Метод класса
    struct Method
    {
        // Имя метода
        std::string name;
        // Имена формальных параметров метода
        std::vector<std::string> formal_params;
        // Тело метода
        std::unique_ptr<Executable> body;
        // Признак того, что данный метод является сопрограммой (крутиной).
        bool is_coroutine = false;
    };

    // Псевдокоманда для служебных целей (посылки уведомлений в ast::PrepareExecute)
    struct PsevdoExecutable : public Executable
    {
        ObjectHolder Execute(Closure& closure, Context& context)
        {
            return ObjectHolder::None();
        }

        const std::string* info_data_ptr;
    };

    // Класс
    class Class : public Object
    {
    public:
        // Создаёт класс с именем name и набором методов methods, унаследованный от класса parent
        // Если parent равен nullptr, то создаётся базовый класс
        explicit Class(std::string name, std::vector<Method> methods, const Class* parent);

        //   Возвращает указатель на метод name или nullptr, если метод с таким именем отсутствует.
        // args_count - требуемое количество формальных параметров у искомого метода. Если этот аргумент < 0,
        // будет найден какой-либо метод с именем name из имеющихся в наличии с любым числом формальных параметров.
        //   Аргумент parent_name указывает имя родительского класса, начиная с которого (от класса parent_name в
        // направлении его предков) будет выполняться поиск целевого метода name. Если аргумент parent_name пуст,
        // поиск выполняется непосредственно от данного класса.
        struct GetMethodRet
        {
            const Method* method = nullptr;
            ThrowMessageNumber error = ThrowMessageNumber::THRM_UNKNOWN;

            GetMethodRet() = default;
            GetMethodRet(const GetMethodRet&) = default;
            GetMethodRet(GetMethodRet&&) = default;
            GetMethodRet(const Method* p_method) : method(p_method)
            {}
            GetMethodRet(ThrowMessageNumber p_error) : error(p_error)
            {}

            GetMethodRet& operator=(const GetMethodRet&) = default;
            GetMethodRet& operator=(GetMethodRet&&) = default;

            operator const Method*() const
            {
                return method;
            }

            bool IsError() const
            {
                return method == nullptr;
            }

            operator bool() const
            {
                return !IsError();
            }

            const Method* operator->() const
            {
                return method;
            }

            friend std::ostream& operator<<(std::ostream& ostr, const GetMethodRet& method_ret)
            {
                ostr << method_ret.method;
                return ostr;
            }
        };
        [[nodiscard]] GetMethodRet GetMethod(const std::string& name, int args_count = -1, const std::string& parent_name = {}) const;
        
        // Возвращает массив пар-описателей методов класса
        [[nodiscard]] std::vector<std::pair<std::string, size_t>> GetMethodsDesc() const;

        // Возвращает имя класса
        [[nodiscard]] const std::string& GetName() const;

        // Возвращает "истину", если данный класс является наследником (потомком) проверяемого класса test_my_parent (в силу
        // симметрии отношения наследования проверяемый класс будет в этом случае предком данного класса).
        // В противном случае функции вернут "ложь".
        // Для второй перегрузки проверяемый класс задается его именем test_class_name.
        [[nodiscard]] bool IsSuccessorOf(const std::string& test_my_parent) const;
        [[nodiscard]] bool IsSuccessorOf(const Class& test_my_parent) const;

        // Выводит в os строку "Class <имя класса>", например "Class cat"
        void Print(std::ostream& os, Context& context) override;

        const void* GetPtr() const override
        {
            return nullptr;
        }

        size_t SizeOf() const override
        {
            return 0;
        }

    private:
        std::string my_name_;
        const Class& parent_;
        std::unordered_multimap<std::string, Method> virtual_method_table_;
    };

    class MYTHLON_INTERPRETER_PUBLIC CommonClassInstance : public Object
    {
    public:
        void Print(std::ostream& os, Context& context) override = 0;
        virtual bool HasMethod(const std::string& method_name, size_t argument_count) const = 0;
        virtual ObjectHolder Call(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                  Context& context, const std::string& parent_name = {}) = 0;
        virtual std::string GetClassName() const = 0; // Возвращает имя данного класса.

        // Анализ отношений родства классов. Методы возвращают "ИСТИНУ", если класс test_my_parent
        // является предком класса, экземпляром которого является данный объект.
        [[nodiscard]] virtual bool IsSuccessorOf(const std::string& test_my_parent) const
        {
            return GetClassName() == test_my_parent;
        }
        
        [[nodiscard]] virtual bool IsSuccessorOf(const Class& test_my_parent) const
        {
            return GetClassName() == test_my_parent.GetName();
        }

        const void* GetPtr() const override
        {
            return nullptr;
        }

        size_t SizeOf() const override
        {
            return 0;
        }
    };

    // Экземпляр класса
    class ClassInstance : public CommonClassInstance
    {
        friend class CoroutineInstance;

    public:
        explicit ClassInstance(const Class& cls);

        /*
         * Если у объекта есть метод __str__, выводит в os результат, возвращённый этим методом.
         * В противном случае в os выводится адрес объекта.
         */
        void Print(std::ostream& os, Context& context) override;

        /*
         * Вызывает у объекта метод method, передавая ему actual_args параметров.
         * Параметр context задаёт контекст для выполнения метода.
         * Вызов метода-сопрограммы выполняется особым способом - создаётся объект статуса сопрограммы, который и возвращается
         * как результат работы данной функции.
         * Если ни сам класс, ни его родители не содержат метод method, метод выбрасывает исключение runtime_error.
         * \param parent_name - имя родительского класса, начиная от которого будет вестить поиск вызываемого метода method.
         *                      Поиск в этом случае выполняется "вверх" (в восходящем направлении), от предкового класса
                                parent_name к вершине иерархии наследования.
                                Если parent_name пуста, то поиск выполняется непосредственно от данного класса.
         */
        ObjectHolder Call(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                          Context& context, const std::string& parent_name = {}) override;

        // Возвращает true, если объект имеет метод method, принимающий argument_count параметров
        [[nodiscard]] bool HasMethod(const std::string& method, size_t argument_count) const override;

        // Возвращает ссылку на Closure, содержащий поля объекта
        [[nodiscard]] Closure& Fields();
        // Возвращает константную ссылку на Closure, содержащую поля объекта
        [[nodiscard]] const Closure& Fields() const;
        // Возвращает имя хранимого внутри класса
        [[nodiscard]] std::string GetClassName() const;

        // Анализ родства классов. Проверка того, являектся ли класс test_my_parent предком класса, к которому
        // относится данный объект.
        [[nodiscard]] virtual bool IsSuccessorOf(const std::string& test_my_parent) const override;
        [[nodiscard]] virtual bool IsSuccessorOf(const Class& test_my_parent) const override;

    private:
        const Class& my_class_;
        Closure closure_;
        std::unique_ptr<PsevdoExecutable> dummy_statement_ = std::make_unique<PsevdoExecutable>();

        const Class& GetBaseClass() const
        {
            return my_class_;
        }
    };

    void MYTHLON_INTERPRETER_PUBLIC CheckMethodParams(Context& context, const std::string& method_name,
                           MethodParamCheckMode check_mode,
                           MethodParamType param_type, size_t required_params,
                           const std::vector<ObjectHolder>& actual_args);

    /*
    * Классы-хранители состояния (точнее, положения) потока управления программы, позволяющий однозначно восстановить ход ее работы
    * от некоторой опорной точки точки до положения, описанного в этой структуре.
    */
    // Отметка о входе потока управления внуть некоторого метода.
    struct MethodWorkflowPosData
    {
        const Method* method = nullptr;
    };

    // Тип хранения положения потока исполнения внутри составного (группового) последовательного оператора Compound.
    struct CompoundWorkflowPosData
    {
        int index = -1;
    };

    // Тип хранения информации о текущем положении потока исполнения внутри структурного оператора if ... else ... .
    struct IfElseWorkflowPosData
    {
        enum class IfElseBranch
        {
            IFELSE_BRANCH_UNKNOWN = 0,
            IFELSE_BRANCH_IF,
            IFELSE_BRANCH_ELSE
        };

        IfElseBranch if_pass_branch = IfElseBranch::IFELSE_BRANCH_UNKNOWN;
    };

    // Тип хранения информации о текущем положении потока исполнения внутри структурного оператора while ... .
    struct WhileWorkflowPosData
    {
        bool is_pass_internal = false;
    };

    using WorkFlowPosData =
        std::variant<std::monostate, MethodWorkflowPosData, CompoundWorkflowPosData,
                     IfElseWorkflowPosData, WhileWorkflowPosData>;

    class WorkflowPosition
    {
    public:
        enum class WorkPosType
        {
            WORK_POS_UNKNOWN = 0,
            WORK_POS_METHOD,        // Исполняется метод класса.
            WORK_POS_COMPOUND,      // Исполняется составной оператор типа Compound.
            WORK_POS_IF_ELSE,       // Исполняется блок if ... else ... .
            WORK_POS_WHILE          // Исполняется блок while.
        };

        WorkflowPosition() = default;
        WorkflowPosition(WorkFlowPosData pos_data, Executable* block_statement) :
            pos_data_(pos_data), block_statement_(block_statement)
        {}

        WorkPosType GetType() const
        {
            if (std::holds_alternative<MethodWorkflowPosData>(pos_data_))
                return WorkPosType::WORK_POS_METHOD;
            else if (std::holds_alternative<CompoundWorkflowPosData>(pos_data_))
                return WorkPosType::WORK_POS_COMPOUND;
            else if (std::holds_alternative<IfElseWorkflowPosData>(pos_data_))
                return WorkPosType::WORK_POS_IF_ELSE;
            else if (std::holds_alternative<WhileWorkflowPosData>(pos_data_))
                return WorkPosType::WORK_POS_WHILE;

            return WorkPosType::WORK_POS_UNKNOWN;
        }

    protected:
        WorkFlowPosData pos_data_;
        Executable* block_statement_ = nullptr;
    };

    class WorkflowStackSaver : public Object
    {
    public:
        void PushBack(WorkflowPosition new_workflow_position)
        {
            workflow_data_.push_back(new_workflow_position);
        }

        WorkflowPosition PopBack(WorkflowPosition::WorkPosType find_pos_type = WorkflowPosition::WorkPosType::WORK_POS_UNKNOWN)
        {
            while (!workflow_data_.empty())
            {
                WorkflowPosition top_workflow_pos = workflow_data_.back();
                workflow_data_.pop_back();
                if (top_workflow_pos.GetType() == find_pos_type ||
                    find_pos_type == WorkflowPosition::WorkPosType::WORK_POS_UNKNOWN)
                {
                    CorrectCurrentIndex();
                    return top_workflow_pos;
                }
            }
            CorrectCurrentIndex();
            return {};
        }

        WorkflowPosition* Current()
        {
            if (current_workflow_index_ != (std::numeric_limits<size_t>::max)() &&
                current_workflow_index_ < workflow_data_.size())
                return &workflow_data_[current_workflow_index_];
            else
                return nullptr;
        }

        WorkflowPosition* Reset()
        {
            current_workflow_index_ = 0;
            CorrectCurrentIndex();
            return Current();
        }

        WorkflowPosition* Advance(int dist)
        {  // Сдвиг нндекса текущей позиции потока управления на dist элементов (вперед или назад).
            int new_index = static_cast<int>(current_workflow_index_) + dist;
            current_workflow_index_ = new_index < 0 ? 0 : static_cast<size_t>(new_index);
            CorrectCurrentIndex();
            return Current();
        }

        WorkflowPosition* Back()
        {
            if (!workflow_data_.empty())
                return &(workflow_data_.back());
            else
                return nullptr;
        }

        void Clear()
        {
            workflow_data_.clear();
            current_workflow_index_ = (std::numeric_limits<size_t>::max)();
        }

        void Print(std::ostream& os, [[maybe_unused]] Context& context) override
        {
            if (Current())
                os << "Поток исполнения в " << static_cast<int>(Current()->GetType());
            else
                os << "Поток исполнения не зафиксирован";
        }

        size_t SizeOf() const override
        {
            return 0;
        }
        
        const void* GetPtr() const override
        {
            return nullptr;
        }

    private:
        void CorrectCurrentIndex()
        {
            if (current_workflow_index_ != (std::numeric_limits<size_t>::max)())
            {
                if (workflow_data_.empty())
                    current_workflow_index_ = (std::numeric_limits<size_t>::max)();
                else
                    if (current_workflow_index_ >= workflow_data_.size())
                        current_workflow_index_ = workflow_data_.size() - 1;
            }
        }

        std::vector<WorkflowPosition> workflow_data_;
        size_t current_workflow_index_ = (std::numeric_limits<size_t>::max)();
    };

#include "special_objects.h"
#include "math_object.h"

    /*
     * Возвращает true, если lhs и rhs содержат одинаковые числа, строки или значения типа Bool.
     * Если lhs - объект с методом __eq__, функция возвращает результат вызова lhs.__eq__(rhs),
     * приведённый к типу Bool. Если lhs и rhs имеют значение None, функция возвращает true.
     * В остальных случаях функция выбрасывает исключение runtime_error.
     *
     * Параметр context задаёт контекст для выполнения метода __eq__
     */
    bool Equal(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context);

    /*
     * Если lhs и rhs - числа, строки или значения bool, функция возвращает результат их сравнения
     * оператором <.
     * Если lhs - объект с методом __lt__, возвращает результат вызова lhs.__lt__(rhs),
     * приведённый к типу bool. В остальных случаях функция выбрасывает исключение runtime_error.
     *
     * Параметр context задаёт контекст для выполнения метода __lt__
     */
    bool Less(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context);
    // Возвращает значение, противоположное Equal(lhs, rhs, context)
    bool NotEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context);
    // Возвращает значение lhs>rhs, используя функции Equal и Less
    bool Greater(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context);
    // Возвращает значение lhs<=rhs, используя функции Equal и Less
    bool LessOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context);
    // Возвращает значение, противоположное Less(lhs, rhs, context)
    bool GreaterOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context);

    // Контекст-заглушка, применяется в тестах.
    // В этом контексте весь вывод перенаправляется в строковый поток вывода output
    struct DummyContext : public Context
    {
        std::ostream& GetOutputStream() override
        {
            return output;
        }
        
        LinkageFunction& GetExternalLinkage() override
        {
            return external_link_;
        }

        bool IsTerminated() override
        {
            return false;
        }

        void SetTerminate() override
        {}

        void Clear() override
        {}

        std::ostringstream output;
        LinkageFunction external_link_;
    };

    // Простой контекст, в нём хранится ссылка на поток, который будет использовать команда print.
    // Также тут будет храниться указатель на звонковую функцию, через которую осуществляется связь
    // с внешним программным комплексом. в который встроен этот скриптовый язык.
    // Вдобавок поместим сюда ещё данные и методы, обеспечивающие поддержку отладки программ с
    // помощью внешнего отладчика.
    class MYTHLON_INTERPRETER_PUBLIC SimpleContext : public Context
    {
    public:
        explicit SimpleContext(std::ostream& output, LinkageFunction external_link = LinkageFunction())
            : output_(output), external_link_(std::move(external_link))
        {}

        std::ostream& GetOutputStream() override
        {
            return output_;
        }

        LinkageFunction& GetExternalLinkage() override
        {
            return external_link_;
        }

        bool IsTerminated() override
        {
            return is_terminate_;
        }

        void SetTerminate() override
        {
            is_terminate_ = true;
        }

        void Clear() override
        {
            is_terminate_ = false;
        }

    private:
        std::ostream& output_;
        LinkageFunction external_link_;
        std::atomic_bool is_terminate_{false};
    };

    [[noreturn]] void MYTHLON_INTERPRETER_PUBLIC
        ThrowRuntimeError(runtime::Executable* exec_obj_ptr, const std::string& except_text);
    [[noreturn]] void MYTHLON_INTERPRETER_PUBLIC
        ThrowRuntimeError(runtime::Executable* exec_obj_ptr, ThrowMessageNumber msg_num);
    [[noreturn]] void MYTHLON_INTERPRETER_PUBLIC
        RethrowRuntimeError(runtime::Executable* exec_obj_ptr, std::runtime_error& orig_runtime_error);
    [[noreturn]] void MYTHLON_INTERPRETER_PUBLIC
        ThrowRuntimeError(Context& context, const std::string& except_text);
    [[noreturn]] void MYTHLON_INTERPRETER_PUBLIC
        ThrowRuntimeError(Context& context, ThrowMessageNumber msg_num);
    [[noreturn]] void MYTHLON_INTERPRETER_PUBLIC
        RethrowRuntimeError(Context& context, std::runtime_error& orig_runtime_error);
}  // namespace runtime

void MYTHLON_INTERPRETER_PUBLIC PrepareExecute(runtime::Executable* exec_obj_ptr, runtime::Closure& closure,
                                               runtime::Context& context);

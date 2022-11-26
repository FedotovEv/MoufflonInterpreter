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

    // Контекст исполнения инструкций Mython
    class Context
    {
    public:
        // Возвращает поток вывода для команд print
        virtual std::ostream& GetOutputStream() = 0;
        virtual LinkageFunction GetExternalLinkage() = 0;
        virtual bool IsTraceMode() = 0;
        virtual void TraceOn() = 0;
        virtual void TraceOff() = 0;

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
    class Object
    {
    public:
        virtual ~Object() = default;
        // выводит в os своё представление в виде строки
        virtual void Print(std::ostream& os, Context& context) = 0;
    };

    // Специальный класс-обёртка, предназначенный для хранения объекта в Mython-программе
    class ObjectHolder
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

        // Возвращает true, если ObjectHolder не пуст
        explicit operator bool() const;
        
        // Модифицирует содержимое объекта, перенацеливая указатель data_ на тот объект,
        //на который указывает data_ внутри аргумента object_holder.
        void ModifyData(const ObjectHolder& object_holder);

    private:
        explicit ObjectHolder(std::shared_ptr<Object> data);
        void AssertIsValid() const;

        std::shared_ptr<Object> data_;
    };

    // Объект-значение, хранящий значение типа T
    template <typename T>
    class ValueObject : public Object
    {
    public:
        ValueObject(T v)  // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)
            : value_(v) {
        }

        void Print(std::ostream& os, [[maybe_unused]] Context& context) override {
            os << value_;
        }

        [[nodiscard]] const T& GetValue() const {
            return value_;
        }

    private:
        T value_;
    };

    class PointerObject : public Object
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

    private:
        ObjectHolder* object_ptr_;
    };

    // Таблица символов, связывающая имя объекта с его значением
    using Closure = std::unordered_map<std::string, ObjectHolder>;

    // Проверяет, содержится ли в object значение, приводимое к True
    // Для отличных от нуля чисел, True и непустых строк возвращается true. В остальных случаях - false.
    bool IsTrue(const ObjectHolder& object);

    // Интерфейс для выполнения действий над объектами Mython
    class Executable
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

    private:
        ProgramCommandDescriptor command_desc_;
    };

    // Строковое значение
    using String = ValueObject<std::string>;
    // Числовое значение
    using Number = ValueObject<int>;

    // Логическое значение
    class Bool : public ValueObject<bool>
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
    };

    // Класс
    class Class : public Object
    {
    public:
        // Создаёт класс с именем name и набором методов methods, унаследованный от класса parent
        // Если parent равен nullptr, то создаётся базовый класс
        explicit Class(std::string name, std::vector<Method> methods, const Class* parent);

        // Возвращает указатель на метод name или nullptr, если метод с таким именем отсутствует
        [[nodiscard]] const Method* GetMethod(const std::string& name) const;

        // Возвращает имя класса
        [[nodiscard]] const std::string& GetName() const;

        // Выводит в os строку "Class <имя класса>", например "Class cat"
        void Print(std::ostream& os, Context& context) override;
    private:
        std::string my_name_;
        const Class& parent_;
        std::unordered_map<std::string, Method> virtual_method_table_;
    };

    class CommonClassInstance : public Object
    {
    public:
        void Print(std::ostream& os, Context& context) override = 0;
        virtual ObjectHolder Call(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                  Context& context) = 0;
    };

    // Экземпляр класса
    class ClassInstance : public CommonClassInstance
    {
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
         * Если ни сам класс, ни его родители не содержат метод method, метод выбрасывает исключение
         * runtime_error
         */
        ObjectHolder Call(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                          Context& context) override;

        // Возвращает true, если объект имеет метод method, принимающий argument_count параметров
        [[nodiscard]] bool HasMethod(const std::string& method, size_t argument_count) const;

        // Возвращает ссылку на Closure, содержащий поля объекта
        [[nodiscard]] Closure& Fields();
        // Возвращает константную ссылку на Closure, содержащую поля объекта
        [[nodiscard]] const Closure& Fields() const;
        // Возвращает имя хранимого внутри класса
        [[nodiscard]] std::string GetClassName() const;

    private:
        const Class& my_class_;
        Closure closure_;
    };

#include "special_objects.h"

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
        
        LinkageFunction GetExternalLinkage() override
        {
            return {};
        }

        bool IsTraceMode() override
        {
            return false;
        }

        void TraceOn() override
        {}

        void TraceOff() override
        {}

        std::ostringstream output;
    };

    // Простой контекст, в нём хранится ссылка на поток, который будет использовать команда print.
    // Также тут будет храниться указатель на звонковую функцию, через которую осуществляется связь
    // с внешним программным комплексом. в который встроен этот скриптовый язык.
    // Вдобавок поместим сюда ещё данные и методы, обеспечивающие поддержку отладки программ с
    // помощью внешнего отладчика.
    class SimpleContext : public Context
    {
    public:
        explicit SimpleContext(std::ostream& output, LinkageFunction external_link = LinkageFunction())
            : output_(output), external_link_(external_link)
        {}

        std::ostream& GetOutputStream() override
        {
            return output_;
        }

        LinkageFunction GetExternalLinkage() override
        {
            return external_link_;
        }

        bool IsTraceMode() override
        {
            return is_trace_mode_;
        }

        void TraceOn() override
        {
            is_trace_mode_ = true;
        }

        void TraceOff() override
        {
            is_trace_mode_ = false;
        }

    private:
        std::ostream& output_;
        LinkageFunction external_link_;
        bool is_trace_mode_ = false;
        std::vector<ProgramCommandDescriptor> breakpoints_;
    };

    [[noreturn]] void ThrowRuntimeError(runtime::Executable* exec_obj_ptr, const std::string& except_text);
    [[noreturn]] void ThrowRuntimeError(runtime::Executable* exec_obj_ptr, ThrowMessageNumber msg_num);
    [[noreturn]] void RethrowRuntimeError(runtime::Executable* exec_obj_ptr, std::runtime_error& orig_runtime_error);
    [[noreturn]] void ThrowRuntimeError(Context& context, const std::string& except_text);
    [[noreturn]] void ThrowRuntimeError(Context& context, ThrowMessageNumber msg_num);
    [[noreturn]] void RethrowRuntimeError(Context& context, std::runtime_error& orig_runtime_error);
}  // namespace runtime

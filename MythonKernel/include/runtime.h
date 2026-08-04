#pragma once

#include "declares.h"
#include "throw_messages.h"
#undef MYTHLON_PLUGIN
#include "plugin_helpers.h"

#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <map>
#include <variant>
#include <atomic>
#include <mutex>
#include <optional>

namespace ast
{
    class CoYield;
    class ClassDefinition;
}

namespace runtime
{
    enum class CommandGenus
    {
        CMD_GENUS_UNKNOWN = 0,
        CMD_GENUS_PRE_FIRST_METHOD_STMT,    // Псевдоинструкция, расположенная непосредственно перед первой действительной командой метода или функции.
        CMD_GENUS_RETURN_FROM_METHOD,       // Инструкция выхода из метода (return, return_ref, co_yield, и.т.д.).
        CMD_GENUS_AFTER_LAST_METHOD_STMT,   // Псевдоинструкция, размещённая после последней действительной команды метода или функции.
        CMD_GENUS_DECLARATIVE,              // Узел АСД декларативной природы, не являющийся непосредственной исполняемой инструкцией.
        CMD_GENUS_INITIALIZE                // (Псевдо)инструкция общей инициализации программы (ProgramCompound).
    };

    // Контекст исполнения инструкций Mython.
    class Context
    {
    public:
        enum class OptionType
        { // Тип опции, запрашиваемой у функции-члена GetOption().
            CONTEXT_OPT_UNKNOWN = 0,
            CONTEXT_OPT_DESTRUCT_AT_FINISH,  // Требуется ли разрушать сохранившиеся объекты в таблице символов при завершении программы.
            CONTEXT_OPT_SKIP_DECLARATIVE,    // Пропускать при отладке (не совершать отладочных звонков) декларативные узлы АСД.
            CONTEXT_OPT_SKIP_FUNC_FRAME      // Пропускать при отладке рамочные (ограничительные) узлы функций и методов.
        };

        Context()
        {
            last_command_desc_ = {.module_id  = -1, .module_string_number  = -1};
        }
        virtual ~Context() = default;
        // Возвращает поток вывода для команд print.
        virtual std::ostream& GetOutputStream() = 0;
        virtual LinkageFunction& GetExternalLinkage() = 0;
        virtual bool IsTerminated() = 0;
        virtual void SetTerminate() = 0;
        virtual void Clear() = 0;
        virtual LinkageValue GetOption(OptionType ask_option) = 0;

        ProgramCommandDescriptor GetLastCommandDesc() const
        {
            return last_command_desc_;
        }

        void SetLastCommandDesc(const ProgramCommandDescriptor& last_command_desc)
        {
            last_command_desc_ = last_command_desc;
        }

    private:
        // Дескриптор последней корректной исполненной команды.
        #ifdef MYTHON_UNITHREAD
            ProgramCommandDescriptor last_command_desc_;
        #else
            std::atomic<ProgramCommandDescriptor> last_command_desc_;
        #endif
    };

    // Базовый класс для всех объектов языка Mython.
    class Object
    {
    public:
        virtual ~Object() = default;
        virtual size_t SizeOf() const = 0;
        virtual const void* GetPtr() const = 0;
        // Выводит в os своё представление в виде строки.
        virtual void Print(std::ostream& os, Context& context) = 0;
    };

    class CommonClassInstance;
    // Специальный класс-обёртка, предназначенный для хранения объекта в Mython-программе.
    // Все хранимые объекты полагаются потомками Object.
    class ObjectHolder
    {
    public:
        // Создаёт пустое значение
        ObjectHolder() = default;

        // Возвращает ObjectHolder, владеющий объектом типа T.
        // Тип T - конкретный класс-наследник Object.
        // object копируется или перемещается в кучу.
        template <typename T>
        [[nodiscard]] static ObjectHolder Own(T&& object)
        {
            return ObjectHolder(std::make_shared<T>(std::forward<T>(object)));
        }

        // Создаёт ObjectHolder, не владеющий объектом (аналог слабой ссылки).
        [[nodiscard]] static ObjectHolder Share(Object& object);
        // Создаёт пустой ObjectHolder, соответствующий значению None.
        [[nodiscard]] static ObjectHolder None();

        // Возвращает ссылку на Object внутри ObjectHolder.
        // ObjectHolder должен быть непустым.
        Object& operator*() const;

        Object* operator->() const;

        [[nodiscard]] Object* Get() const noexcept;

        // Возвращает указатель на объект типа T либо nullptr, если внутри ObjectHolder не хранится
        // объект данного типа.
        template <typename T>
        [[nodiscard]] T* TryAs() const
        {
            return dynamic_cast<T*>(this->Get());
        }

        // Возвращает true, если ObjectHolder не пуст.
        explicit operator bool() const noexcept;

        // Возвращает "ИСТИНУ", если указатель владеющий.
        bool IsOwning() const noexcept;
        
        // Модифицирует содержимое объекта, перенацеливая указатель data_ на тот объект,
        // на который указывает data_ внутри аргумента object_holder.
        void ModifyData(const ObjectHolder& object_holder);

        // Возврат количества ссылок на объект, указатель на который хранится в данном вместилище.
        long UseCount() const noexcept;
        
    private:
        explicit ObjectHolder(std::shared_ptr<Object> data);
        bool IsOwning(const std::shared_ptr<Object>& test_ptr) const noexcept;
        void AssertIsValid() const;

        // Пустой удалитель, применяемый для невладеющих вместилищ, возвращаемых методом Share().
        static void EmptyDeleter(Object*) noexcept
        {}  // Не делает ничего. Абсолютно ничего.

        std::shared_ptr<Object> data_;
    };

    // Объект-контейнер, предназначенный для хранения внутри себя одного из конкретных классов ошибки (CommonError или его наследники).
    // Именно такой объект выбрасывается вместе с исключением оператором raise или внутренними функциями среды исполнения Муфлоно-программы,
    // которая обеспечивается для неё интерпретатором, при возникновении любых ошибок или исключений периода исполнения.
    struct RuntimeError : public std::runtime_error
    {
        static std::string ExtractMessage(const runtime::ObjectHolder& error_object);

        RuntimeError(runtime::ObjectHolder error_object = {}) : 
            std::runtime_error(ExtractMessage(error_object)), error_object_(std::move(error_object))
        {}
        RuntimeError(const std::string& error_message) : std::runtime_error(error_message)
        {}

        // Возвращает true, если контейнер не пуст (то есть содержит внутри себя некоторую конкретную ошибку).
        explicit operator bool() const
        {
            return static_cast<bool>(error_object_);
        }

        runtime::ObjectHolder error_object_ = {}; // Здесь размещается какой-либо из конкретных классов ошибки.
    };

    // Объект-значение, хранящий значение типа T.
    template <typename T>
    class ValueObject : public Object
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

    // Проверяет, содержится ли в object значение, приводимое к True.
    // Для отличных от нуля чисел, True и непустых строк возвращается true. В остальных случаях - false.
    bool IsTrue(const ObjectHolder& object);

    // Универсальный интерфейс общего исполняемого объекта языка МУФЛОН. Именно из таких объектов (их различных частных
    // разновидностей) строится и состоит абстрактное синтаксическое дерево (АСД) разобранной МУФЛОН-программы.
    class Executable
    {
    public:
        Executable() = default;
        virtual ~Executable() = default;
        // Выполняет действие над объектами внутри closure, используя context.
        // Возвращает результирующее значение либо None.
        virtual ObjectHolder Execute(Closure& closure, Context& context) = 0;
        ProgramCommandDescriptor GetCommandDesc()
        {
            return command_desc_;
        }

        void SetCommandDesc(const ProgramCommandDescriptor& command_desc)
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

    // Специальный исполняемый объект, применяемый при построении различных тестов, как других исполняемых объектов,
    // так и объектов периода исполнения.
    struct TestMethodBody : public Executable
    {
        using Fn = std::function<ObjectHolder(Closure& closure, Context& context)>;
        Fn body;

        explicit TestMethodBody(Fn body) : body(std::move(body))
        {}

        ObjectHolder Execute(Closure& closure, Context& context) override
        {
            if (body)
                return body(closure, context);
            return {};
        }
    };

    // Особый тип двухступенчатых исполняемых объектов, имеющих "левую" и "правую" части, которые могут исполняться
    // раздельно (типа оператора присваивания).
    class LeftRightExecutable : public Executable
    {
    public:
        // Отдельный исполнитель "правой" части инструкции.
        virtual ObjectHolder ExecuteRight(Closure& closure, Context& context) = 0;
        // Отдельный исполнитель "левой" части инструкции. right_value - результат, возвращённый "правой" частью оператора.
        virtual ObjectHolder ExecuteLeft(ObjectHolder&& right_value, Closure& closure, Context& context) = 0;
    };

    // Строковое значение
    class String : public ValueObject<std::string>
    {
    public:
        using ValueObject<std::string>::ValueObject;

        String(const String& other) = default;
        String(String&& other) = default;
        // Введём также операторы присваивания - в некоторых случаях они также могут быть нам полезны.
        String& operator=(const String& other) = default;
        String& operator=(String&& other) = default;

        const void* GetPtr() const
        {
            return GetValue().data();
        }

        size_t SizeOf() const
        {
            return GetValue().size();
        }

        // Расширенные атрибуты текстовой строки, обеспечивающие поддержку работу со строковыми кодировками.
        // Указатель на информацию о кодировке данной строки.
        const SingleByteEncodingDesc* encoding = nullptr;
        // Карта индексов, по которым размещаются многобайтовые UTF-8-коды символов строки, если она использует
        // именно UTF-8-кодировку.
        UTF8Map utf8_map;
        // ---- Далее следуют некоторые поля, указывающие на настройки, которые могут быть сделаны для каждого строкового
        // значения индивидуально. ----
        // Локальный перечень сравнительных весов. Если назначен, имеет преимущество над указанным в спецификации кодировки.
        std::string collate;
        // Группа флагов режима сравнительных действий над содержимым строки. 
        // Флаг применения "весов" символов при сравнительных действиях над строками. Если поле установлено в "ИСТИНУ",
        // сравнения символов выполняются с учётом их "веса". Иначе веса игнорируются и сравниваются непосредственно коды
        // символов.
        bool is_use_collate = true;
        // Флаг-уточнитель способа применения значения сравнительных весов символов при сравнении строк на точное равенство.
        // Если оба флага (is_use_collate и is_equal_collate) установлены в "ИСТИНУ", сравнительные веса (содержимое
        // приоритетного поля collate) при проверке равенства символов применяются. При любом другом сочетании флагов для
        // определения точного равенства символов принимается во внимание только их код, а "вес" - игнорируется.
        bool is_equal_collate = true;
        // Если флаг установлен в "ИСТИНУ", сравнения производятся без учёта регистра символов.
        bool is_case_indep_compare = false;
    };

    // Далее описываются структуры, необходимые для работы с числовыми значениями.
    class Number : public Object
    {
    public:
        Number(NumberValue v) : value_(v)
        {}

        Number(int v) : value_(v)
        {}

        Number(double v) : value_(v)
        {}

        Number(const Number& other) = default;
        Number(Number&& other) = default;
        // Для этого класса введём также операторы присваивания - в некоторых случаях они также могут быть нам полезны.
        Number& operator=(const Number& other) = default;
        Number& operator=(Number&& other) = default;

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
    class Bool : public ValueObject<bool>
    {
    public:
        using ValueObject<bool>::ValueObject;

        void Print(std::ostream& os, Context& context) override;
    };

    // Метод класса или свободная функция.
    struct Method
    {
        // Система конструкторов и операторов присваивания здесь необходима, так как члену body требуется всегда
        // актуальная ссылка на саму структуру Method, которой он принадлежит. И эти специальные методы как раз и будут
        // такую ссылку обновлять.
        Method() = default;
        Method(std::string p_name, std::vector<std::string> p_formal_params, std::unique_ptr<Executable> p_body, bool p_is_coroutine = false);
        Method(const Method& other) = delete;
        Method(Method&& other) noexcept;
        // Операторы присваивания.
        Method& operator=(const Method& other) = delete;
        Method& operator=(Method&& other) noexcept;

        // Имя метода (функции).
        std::string name;
        // Имена формальных параметров метода.
        std::vector<std::string> formal_params;
        // Тело метода
        std::unique_ptr<Executable> body;
        // Признак того, что данный метод является сопрограммой (крутиной).
        bool is_coroutine = false;

        void TuneBodyReference();
    };

    // Псевдокоманда для служебных целей (посылки уведомлений в ast::PrepareExecute)
    struct PsevdoExecutable : public Executable
    {
        ObjectHolder Execute(Closure& closure, Context& context)
        {
            return ObjectHolder::None();
        }

        void* info_data_ptr = nullptr;
    };

    // Класс
    class Class : public Object
    {
    public:
        // Создаёт класс с именем name и набором методов methods, унаследованный от класса parent
        // Если parent равен nullptr, то создаётся базовый класс
        // explicit Class(std::string name, std::vector<Method> methods, const Class* parent);
        explicit Class(std::string name, std::vector<Method> methods, std::vector<const Class*> parents);

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

        int GetId() const
        {
            return my_id_;
        }

    private:
        int my_id_;
        std::string my_name_;
        // Список ссылок на ближайших предков данного класса (ближайших по восходящей линии в иерархии наследования).
        using ParentRefType = std::reference_wrapper<const Class>;
        std::vector<ParentRefType> parents_;
        // Таблица методов ("виртуальных") класса. Допускает множество методов с одинаковым именем (далее они различаются
        // по количеству аргументов).
        std::unordered_multimap<std::string, Method> virtual_method_table_;

        // Функция-член, выполняющая обход дерева предков данного класса, вызывая для каждого узла такого дерева обработчик
        // handle_parent_func (вариант внутреннего итератора для такого дерева).
        bool TraverseParents(std::function<bool(const Class&)> handle_parent_func) const;
    };

    // Объект, хранящий описание свободной функции, определённой в программе, в таком виде, в котором его можно упаковать
    // во вместилище ObjectHolder.
    class FreeFunction : public Object
    {
        friend class CoroutineInstance;
        friend class TypeTraitsInstance;

    public:
        explicit FreeFunction(Method method_func);

        const void* GetPtr() const override
        {
            return nullptr;
        }

        size_t SizeOf() const override
        {
            return 0;
        }

        void Print(std::ostream& os, Context& context);

        // Функции-члены класса, исполняющие хранимую функцию.
        ObjectHolder Call(const std::vector<ObjectHolder>& actual_args, Context& context);
        ObjectHolder ExecuteBody(Closure& closure, Context& context);

        // Информирующие функции-члены.
        std::string GetName() const;
        size_t GetArgCount() const;
        bool IsCoroutine() const;

    private:
        Method method_func_;
    };

    // Абстрактный чисто виртуальный класс, выражающий сущность экземпляра "обобщённого" класса, как программно определённого (структура
    // ClassInstance, представляет собой экземпляр класса, определённого непосредственно в МУФЛОН-программе), так и специального (определённого
    // прямо в исходном коде данного интерпретатора, классы типа ArrayInstance, MapInstance, MathInstance, и. т. д.).
    class CommonClassInstance : public Object
    {
    public:
        void Print(std::ostream& os, Context& context) override = 0;
        virtual bool HasMethod(const std::string& method_name, size_t argument_count, const std::string& parent_name = {}) const = 0;
        virtual ObjectHolder Call(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                  Context& context, const std::string& parent_name = {}) = 0;
        virtual std::string GetClassName() const = 0; // Возвращает имя данного класса.

        // Анализ отношений родства классов. Методы возвращают "ИСТИНУ", если класс test_my_parent
        // является предком класса, экземпляром которого является данный объект.
        [[nodiscard]] virtual bool IsSuccessorOf(const std::string& test_my_parent) const
        { // Это тривиальная реализация по умолчанию. Базовый класс иерархии не имеет иных предков, кроме самого себя.
            return GetClassName() == test_my_parent;
        }
        
        [[nodiscard]] virtual bool IsSuccessorOf(const Class& test_my_parent) const
        { // Это также тривиальная реализация по умолчанию. Базовый класс может быть потомком только себя самого.
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

    // Экземпляр класса.
    class ClassInstance : public CommonClassInstance
    {
        friend class CoroutineInstance;
        friend class TypeTraitsInstance;

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

        // Возвращает true, если объект имеет метод method, принимающий argument_count параметров.
        [[nodiscard]] bool HasMethod(const std::string& method, size_t argument_count, const std::string& parent_name = {}) const override;

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

        const Class& GetBaseClass() const
        {
            return my_class_;
        }
    };

    void CheckMethodParams(Context& context, const std::string& method_name, MethodParamCheckMode check_mode,
                           MethodParamType param_type, size_t required_params, const std::vector<ObjectHolder>& actual_args);

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

    // Тип хранения информации о текущем положении потока исполнения внутри структурного оператора if...elif...else ... .
    struct IfElseWorkflowPosData
    {
        int index = -1; // Индекс (базированный к нулю) ветви (варианта) оператора if...elif...else, которая в данный момент исполняется.
    };


    // Тип хранения информации о текущем положении потока исполнения внутри структурного оператора while ... .
    struct WhileWorkflowPosData
    {
        bool is_pass_internal = false;
    };

    // Хранитель информации о состоянии исполнения операторов приостановки сопрограмм co_await, co_yield и co_yield_ref.
    struct CoYieldWorkflowPosData
    {
        bool is_already_executed = false;
    };

    // Хранитель информации о потоке управления внутри структуры try ... except ... else ... finally ... .
    struct TryExceptWorkflowPosData
    {
        enum class TryExceptBranch
        {
            TRYEXCEPT_BRANCH_UNKNOWN = 0,
            TRYEXCEPT_BRANCH_TRY,
            TRYEXCEPT_BRANCH_ANY_EXCEPT,
            TRYEXCEPT_BRANCH_NAMED_EXCEPT,
            TRYEXCEPT_BRANCH_ANONYMOUS_EXCEPT,
            TRYEXCEPT_BRANCH_ELSE,
            TRYEXCEPT_BRANCH_FINALLY
        };

        TryExceptBranch try_except_pass_branch = TryExceptBranch::TRYEXCEPT_BRANCH_UNKNOWN;
        int index = -1; // Индекс (порядковый номер, отсчитываемый от нуля), указывающий на активный except-кадр,
                        // внутри которого находится управление в момент сохранения кадра.
        RuntimeError runtime_error_object_; // Объект-контейнер ошибки, выброшенный вместе с исключением в try-блоке.
        Closure except_block_closure;       // Специальная (дополненная) таблица символов, применяемая при вызове именованного except-блока.
    };

    using WorkFlowPosData =
        std::variant<std::monostate, MethodWorkflowPosData, CompoundWorkflowPosData, IfElseWorkflowPosData,
                     WhileWorkflowPosData, CoYieldWorkflowPosData, TryExceptWorkflowPosData>;

    class WorkflowPosition
    {
    public:
        enum class WorkPosType
        {
            WORK_POS_UNKNOWN = 0,
            WORK_POS_METHOD,        // Исполняется метод класса.
            WORK_POS_COMPOUND,      // Исполняется составной оператор типа Compound.
            WORK_POS_IF_ELSE,       // Исполняется блок if ... elif ... else ... .
            WORK_POS_WHILE,         // Исполняется блок while.
            WORK_POS_CO_YIELD,      // Исполняется какой-либо оператор приостановки сопрограммы - co_yield ... или co_yield_ref ... .
            WORK_POS_TRY_EXCEPT     // Производится выполнение блока try ... except ... .
        };

        WorkflowPosition() = default;
        WorkflowPosition(WorkFlowPosData pos_data, Executable* block_statement) :
            pos_data_(pos_data), block_statement_(block_statement)
        {}

        WorkPosType GetType() const;
        WorkFlowPosData& GetData()
        {
            return pos_data_;
        }

        Executable* GetOwningStatement()
        {
            return block_statement_;
        }

    protected:
        WorkFlowPosData pos_data_;
        Executable* block_statement_ = nullptr;
    };

    class WorkflowStackSaver : public Object
    {
    public:
        WorkflowPosition* PushBack(WorkflowPosition new_workflow_position);
        WorkflowPosition PopBack(WorkflowPosition::WorkPosType find_pos_type = WorkflowPosition::WorkPosType::WORK_POS_UNKNOWN);
        WorkflowPosition* Current();
        WorkflowPosition* SetIndex(int new_index = 0);
        WorkflowPosition* Advance(int dist = 1); // Сдвиг нндекса текущей позиции потока управления на dist элементов (вперед или назад).
        WorkflowPosition* Back();
        void Clear();
        void Print(std::ostream& os, [[maybe_unused]] Context& context) override;

        size_t SizeOf() const override
        {
            return 0;
        }
        
        const void* GetPtr() const override
        {
            return nullptr;
        }

    private:
        void CorrectCurrentIndex();

        std::list<WorkflowPosition> workflow_data_;
        int current_workflow_index_ = 0;
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

        LinkageValue GetOption(OptionType ask_option) override
        {
            return {};
        }

        std::ostringstream output;
        LinkageFunction external_link_;
    };

    // Простой контекст, в нём хранится ссылка на поток, который будет использовать команда print.
    // Также тут будет храниться указатель на звонковую функцию, через которую осуществляется связь
    // с внешним программным комплексом. в который встроен этот скриптовый язык.
    // Вдобавок поместим сюда ещё данные и методы, обеспечивающие поддержку отладки программ с
    // помощью внешнего отладчика.
    class SimpleContext : public Context
    {
    public:
        explicit SimpleContext(std::ostream& output, LinkageFunction external_link = LinkageFunction());
        SimpleContext(const SimpleContext& other) = delete;
        SimpleContext(SimpleContext&& other) noexcept;

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

        LinkageValue GetOption(OptionType ask_option) override;
        bool SetOption(OptionType set_option, const LinkageValue& option_value);

    private:
        std::ostream& output_;
        LinkageFunction external_link_;
        #ifndef MYTHON_UNITHREAD
            // Поддержка потокобезопасности при синхронном доступе.
            std::atomic_bool is_terminate_{false};
            mutable std::mutex opt_mutex_;
        #else
            // В этом варианте примитивы многопоточности не используются.
            bool is_terminate_{false};
        #endif
        std::unordered_map<OptionType, LinkageValue> opt_data_;
    };
    
    std::string CommandGenusToString(CommandGenus cmd_genus);
    std::ostream& operator<<(std::ostream& ostr, CommandGenus cmd_genus);
}  // namespace runtime

void PrepareExecute(runtime::Executable* exec_obj_ptr, runtime::Closure& closure, runtime::Context& context);

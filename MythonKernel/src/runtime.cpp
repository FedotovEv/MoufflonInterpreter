
#include "runtime.h"
#include "parse.h"
#include "error_classes.h"
#include "statement.h"
#include "encodings.h"
#include "math_object.h"

#include <cassert>
#include <optional>
#include <sstream>
#include <queue>
#include <codecvt>
#include <locale>

using namespace std;

extern const std::vector<std::pair<char, char>> empty_upcase_table;
extern const std::string empty_collate;

namespace runtime
{
    // Возврат действительной длины строки в символах.
    size_t String::SymbolSizeOf() const
    {
        if (encoding == UTF_8_ENCODING)
            return utf8_map.SymbolSizeOf();
        else
            return GetValue().size();
    }

    // Функция возврата байтовой позицию сразу за концом корректной части UTF-8-строки.
    size_t String::BytePosAfterEnd() const
    {
        return utf8_map.BytePosAfterEnd();
    }

    // Возвращает байтовую позицию символа с индексом symb_index.        
    size_t String::SymbolBytePos(size_t symb_index) const
    {
        return utf8_map.SymbolBytePos(symb_index);
    }

    // Расчёт байтовой длины (длины в байтах) кода символа с индексом symb_index.
    size_t String::SymbolByteSize(size_t symb_index) const
    {
        return utf8_map.SymbolByteSize(symb_index);
    }

    // Поиск символа с кодом длиной symb_code_size, размещённым в строке symb_code_str в позиции symb_code_pos.
    size_t String::FindSymbol(const std::string& symb_code_str, size_t symb_code_pos, size_t symb_code_size, size_t start_pos) const
    {
        if (encoding != UTF_8_ENCODING)
        { // Поиск символа для однобайтовой кодировки.
            if (symb_code_size != 1 || symb_code_pos >= symb_code_str.size())
                return std::string::npos;   // Для однобайтовых кодировок символы тоже могут быть только длиной в 1 байт.
            return GetValue().find(symb_code_str[symb_code_pos], start_pos);
        }
        else
        { // Поиск символа для многобайтовой кодировки UTF-8.
            if (symb_code_size == 0 || symb_code_size > MAX_UNICODE_LENGTH || symb_code_pos >= symb_code_str.size())
                // Недопустимая длина UTF-8-кода или некорректно указано положение искомого символа в строке symb_code_str.
                return std::string::npos;
            const std::string& current_str_value = GetValue();
            for (size_t test_symb_index = start_pos; test_symb_index < SymbolSizeOf(); ++test_symb_index)
            {
                if (current_str_value.compare(SymbolBytePos(test_symb_index), SymbolByteSize(test_symb_index),
                                              symb_code_str, symb_code_pos, symb_code_size) == 0)
                    return test_symb_index;
            }
            return std::string::npos;
        }
    }

    // Поиск символа symb_code в данной строке и возврат его СИМВОЛЬНОГО положения (порядкового индекса как символа).
    size_t String::FindSymbol(uint32_t symb_code, size_t start_pos) const
    {
        if (encoding != UTF_8_ENCODING)
        { // Поиск символа для однобайтовой кодировки.
            if (symb_code > 0xffu)
                return std::string::npos;
            return GetValue().find(*reinterpret_cast<char*>(&symb_code), start_pos);
        }
        else
        { // Поиск символа для многобайтовой кодировки UTF-8.
            std::string symb_code_str = ConvSymbToUTF8(symb_code);
            const std::string& current_str_value = GetValue();
            for (size_t test_symb_index = start_pos; test_symb_index < SymbolSizeOf(); ++test_symb_index)
            {
                if (current_str_value.compare(SymbolBytePos(test_symb_index), SymbolByteSize(test_symb_index), symb_code_str) == 0)
                    return test_symb_index;
            }
            return std::string::npos;
        }
    }

    const std::vector<std::pair<char, char>>& String::GetUpcaseTable() const
    {
        if (encoding != NO_ENCODING && encoding != UTF_8_ENCODING)
            return encoding->upcase_table;
        return empty_upcase_table;
    }

    const std::string& String::GetCollate() const
    {
        // Локальная взвешивающая строка имеет преимущество над кодировочно-специфичной.
        if (collate.size() == COLLATE_SIZE)
            return collate;

        if (encoding != NO_ENCODING && encoding != UTF_8_ENCODING)
        {
            if (encoding->IsCollateValid())
                return encoding->collate;
        }
        // Приемлемая строка весов отсутствует в обоих возможных источниках.
        return empty_collate;
    }

    std::string RuntimeError::ExtractMessage(const runtime::ObjectHolder& error_object)
    {
        if (const runtime::CommonClassInstance* error_class_ptr = error_object.TryAs<runtime::CommonClassInstance>())
        {
            if (const CommonError* common_error = dynamic_cast<const CommonError*>(error_class_ptr))
                return common_error->GetText();
        }
        return {};
    }

    ObjectHolder::ObjectHolder(std::shared_ptr<Object> data) : data_(std::move(data))
    {}

    void ObjectHolder::AssertIsValid() const
    {
        assert(data_ != nullptr);
    }

    ObjectHolder ObjectHolder::Share(Object& object)
    {
        // Возвращаем невладеющий shared_ptr (его удалитель ничего не делает).
        return ObjectHolder(std::shared_ptr<Object>(&object, EmptyDeleter));
    }

    ObjectHolder ObjectHolder::None()
    {
        return ObjectHolder();
    }

    Object& ObjectHolder::operator*() const
    {
        AssertIsValid();
        return *Get();
    }

    Object* ObjectHolder::operator->() const
    {
        AssertIsValid();
        return Get();
    }

    Object* ObjectHolder::Get() const noexcept
    {
        return data_.get();
    }

    void ObjectHolder::ModifyData(const ObjectHolder& object_holder)
    {
        data_ = object_holder.data_;
    }

    long ObjectHolder::UseCount() const noexcept
    {
        return data_.use_count();
    }

    ObjectHolder::operator bool() const noexcept
    {
        return Get() != nullptr;
    }

    bool ObjectHolder::IsOwning() const noexcept
    {
        return IsOwning(data_);
    }

    bool ObjectHolder::IsOwning(const std::shared_ptr<Object>& test_ptr) const noexcept
    {
        if (!test_ptr)
            return false;
        if (auto data_del_p = std::get_deleter<void(*)(Object*)>(test_ptr))
            return *data_del_p != EmptyDeleter;
        else
            return true;
    }

    bool IsTrue(const ObjectHolder& object)
    {
        if (Number* number_ptr = object.TryAs<Number>())
        {
            const NumberValue& number_value = number_ptr->GetValue();
            if (holds_alternative<int>(number_value))
                return get<int>(number_value);
            else if (holds_alternative<double>(number_value))
                return get<double>(number_value);
        }
    
        if (String* string_ptr = object.TryAs<String>())
            return string_ptr->GetValue().size();

        if (Bool* bool_ptr = object.TryAs<Bool>())
            return bool_ptr->GetValue();

        return false;
    }

    Number operator+(const Number& first_op, const Number& second_op)
    {
        if (first_op.IsInt() && second_op.IsInt())
        {
            int int_result = first_op.GetIntValue() + second_op.GetIntValue();
            return Number(int_result);
        }
        else
        {
            double double_result = first_op.GetDoubleValue() + second_op.GetDoubleValue();
            return Number(double_result);
        }
    }
    
    Number operator-(const Number& first_op, const Number& second_op)
    {
        if (first_op.IsInt() && second_op.IsInt())
        {
            int int_result = first_op.GetIntValue() - second_op.GetIntValue();
            return Number(int_result);
        }
        else
        {
            double double_result = first_op.GetDoubleValue() - second_op.GetDoubleValue();
            return Number(double_result);
        }
    }
    
    Number operator*(const Number& first_op, const Number& second_op)
    {
        if (first_op.IsInt() && second_op.IsInt())
        {
            int int_result = first_op.GetIntValue() * second_op.GetIntValue();
            return Number(int_result);
        }
        else
        {
            double double_result = first_op.GetDoubleValue() * second_op.GetDoubleValue();
            return Number(double_result);
        }
    }
    
    Number operator/(const Number& first_op, const Number& second_op)
    {
        if (first_op.IsInt() && second_op.IsInt())
        {
            int int_result = first_op.GetIntValue() / second_op.GetIntValue();
            return Number(int_result);
        }
        else
        {
            double double_result = first_op.GetDoubleValue() / second_op.GetDoubleValue();
            return Number(double_result);
        }
    }
    
    Number operator%(const Number& first_op, const Number& second_op)
    {
        if (first_op.IsInt() && second_op.IsInt())
        {
            int int_result = first_op.GetIntValue() % second_op.GetIntValue();
            return Number(int_result);
        }
        else
        {
            int quotient = static_cast<int>(first_op.GetDoubleValue() / second_op.GetDoubleValue());
            double double_result = first_op.GetDoubleValue() - quotient * second_op.GetDoubleValue();
            return Number(double_result);
        }
    }

    Number operator|(const Number& first_op, const Number& second_op)
    {
        if (first_op.IsInt() || second_op.IsInt())
        {
            int int_result = first_op.GetIntValue() | second_op.GetIntValue();
            return Number(int_result);
        }
        else
        {
            double first_temp_double = first_op.GetDoubleValue();
            double second_temp_double = second_op.GetDoubleValue();
            uint64_t result_uint = (*reinterpret_cast<uint64_t*>(&first_temp_double)) |
                                   (*reinterpret_cast<uint64_t*>(&second_temp_double));
            return Number(*reinterpret_cast<double*>(&result_uint));
        }
    }
    
    Number operator&(const Number& first_op, const Number& second_op)
    {
        if (first_op.IsInt() || second_op.IsInt())
        {
            int int_result = first_op.GetIntValue() & second_op.GetIntValue();
            return Number(int_result);
        }
        else
        {
            double first_temp_double = first_op.GetDoubleValue();
            double second_temp_double = second_op.GetDoubleValue();
            uint64_t result_uint = (*reinterpret_cast<uint64_t*>(&first_temp_double)) &
                (*reinterpret_cast<uint64_t*>(&second_temp_double));
            return Number(*reinterpret_cast<double*>(&result_uint));
        }
    }
    
    Number operator^(const Number& first_op, const Number& second_op)
    {
        if (first_op.IsInt() || second_op.IsInt())
        {
            int int_result = first_op.GetIntValue() ^ second_op.GetIntValue();
            return Number(int_result);
        }
        else
        {
            double first_temp_double = first_op.GetDoubleValue();
            double second_temp_double = second_op.GetDoubleValue();
            uint64_t result_uint = (*reinterpret_cast<uint64_t*>(&first_temp_double)) ^
                (*reinterpret_cast<uint64_t*>(&second_temp_double));
            return Number(*reinterpret_cast<double*>(&result_uint));
        }
    }
    
    Number operator<<(const Number& first_op, const Number& second_op)
    {
        if (first_op.IsInt() || second_op.IsInt())
        {
            int int_result = first_op.GetIntValue() << second_op.GetIntValue();
            return Number(int_result);
        }
        else
        {
            double first_temp_double = first_op.GetDoubleValue();
            uint64_t result_uint = (*reinterpret_cast<uint64_t*>(&first_temp_double) <<
                                    second_op.GetIntValue());
            return Number(*reinterpret_cast<double*>(&result_uint));
        }
    }

    Number operator>>(const Number& first_op, const Number& second_op)
    {
        if (first_op.IsInt() || second_op.IsInt())
        {
            int int_result = first_op.GetIntValue() >> second_op.GetIntValue();
            return Number(int_result);
        }
        else
        {
            double first_temp_double = first_op.GetDoubleValue();
            uint64_t result_uint = (*reinterpret_cast<uint64_t*>(&first_temp_double) >>
                second_op.GetIntValue());
            return Number(*reinterpret_cast<double*>(&result_uint));
        }
    }

    Number operator~(const Number& first_op)
    {
        if (first_op.IsInt())
        {
            return Number(~first_op.GetIntValue());
        }
        else
        {
            double temp_double = first_op.GetDoubleValue();
            uint64_t result_uint = ~(*reinterpret_cast<uint64_t*>(&temp_double));
            return Number(*reinterpret_cast<double*>(&result_uint));
        }
    }

    bool operator<(const Number& first_op, const Number& second_op)
    {
        if (first_op.IsInt() && second_op.IsInt())
            return first_op.GetIntValue() < second_op.GetIntValue();
        else
            return first_op.GetDoubleValue() < second_op.GetDoubleValue();
    }
    
    bool operator==(const Number& first_op, const Number& second_op)
    {
        if (first_op.IsInt() && second_op.IsInt())
            return first_op.GetIntValue() == second_op.GetIntValue();
        else
            return first_op.GetDoubleValue() == second_op.GetDoubleValue();
    }

    FreeFunction::FreeFunction(Method method_func) : method_func_(move(method_func))
    {}

    void FreeFunction::Print(std::ostream& os, Context& context)
    {
        os << "FreeFunction " << method_func_.name;
        if (method_func_.is_coroutine)
            os << " - coro";
    }
    
    ObjectHolder FreeFunction::Call(const std::vector<ObjectHolder>& actual_args, Context& context)
    {
        Closure temp_closure;
        // Создаём для вызова свободной функции специальную версию таблицы символов.
        // У свободной функции не будет доступа ни к каким переменным, кроме собственных локальных, которые она будет создавать
        // сама по ходу собственного выполнения, а также фактическим её параметрам, переданным нам через массив actual_args.
        // И именно эти фактические параметры нужно будет сейчас добавить в формируемую таблицу temp_closure.
        if (method_func_.formal_params.size() != actual_args.size())
        {
            std::string err_mess = "Функция " + method_func_.name + ": требуется " + std::to_string(method_func_.formal_params.size()) +
                                   " параметров, передано " + std::to_string(actual_args.size());
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT, err_mess);
        }

        for (size_t param_index = 0; param_index < method_func_.formal_params.size(); ++param_index)
            temp_closure[method_func_.formal_params[param_index]] = actual_args[param_index];
        // Таблица символов подготовлена, можно обработать тело функции.
        if (method_func_.is_coroutine)
        { // Запуск функции как сопрограммы. Она пока только готовится к запуску и будет находиться в приостановленном состоянии.
            return ObjectHolder::Own(move(CoroutineInstance(this, temp_closure)));
        }
        else
        { // Немедленное исполнение обычной функции - непосредственное исполнение и последующее возвращение результата её работы.
            return method_func_.body->Execute(temp_closure, context);
        }
    }

    ObjectHolder FreeFunction::ExecuteBody(Closure& closure, Context& context)
    {
        return method_func_.body->Execute(closure, context);
    }

    std::string FreeFunction::GetName() const
    {
        return method_func_.name;
    }

    size_t FreeFunction::GetArgCount() const
    {
        return method_func_.formal_params.size();
    }

    bool FreeFunction::IsCoroutine() const
    {
        return method_func_.is_coroutine;
    }

    void ClassInstance::Print(std::ostream& os, Context& context)
    {
        if (HasMethod(STR_FUNCTION_METHOD, 0))
            Call(STR_FUNCTION_METHOD, {}, context)->Print(os, context);
        else
            os << this;
    }

    bool ClassInstance::HasMethod(const std::string& method_name, size_t argument_count, const std::string& parent_name) const
    {
        if (const Method* method_ptr = my_class_.GetMethod(method_name, static_cast<int>(argument_count), parent_name))
            if (method_ptr->formal_params.size() == argument_count)
                return true;
            else
                return false;
        else
            return false;
    }

    Closure& ClassInstance::Fields()
    {
        return closure_;
    }

    const Closure& ClassInstance::Fields() const
    {
        return closure_;
    }

    ClassInstance::ClassInstance(const Class& cls) : my_class_(cls)
    {}

    ObjectHolder ClassInstance::Call(const std::string& method_name,
                                     const std::vector<ObjectHolder>& actual_args,
                                     Context& context, const std::string& parent_name)
    {
        Class::GetMethodRet get_method = my_class_.GetMethod(method_name, static_cast<int>(actual_args.size()), parent_name);
        if (!get_method || get_method.method->formal_params.size() != actual_args.size())
            ThrowRuntimeError(context, get_method.error, ThrowMessages::GetThrowText(get_method.error));
    
        Closure method_closure; // Временная таблица символов, применяемая во время исполнения вызываемого метода.
        // Добавляем в эту таблицу ссылку на наш собственный класс под именем SELF_FIELD_NAME("self"), что обеспечивает коду метода
        // доступ к текущим полям объекта.
        method_closure[SELF_FIELD_NAME] = ObjectHolder::Share(*this);
        // Кроме того, создаём в этой временной таблице переменные с именами формальных и значениями фактических параметров метода.
        // Такая подстановка делает передаваемые аргументы доступными исполняемому коду метода.
        auto actual_args_it = actual_args.begin();
        for (const string& formal_param_name : get_method.method->formal_params)
            method_closure[formal_param_name] = *actual_args_it++;

        if (get_method.method->is_coroutine)
        { // Запуск сопрограммы. Она пока только готовится к запуску и будет находиться в приостановленном состоянии.
            return ObjectHolder::Own(move(CoroutineInstance(this, get_method.method, method_closure)));
        }
        else
        { // Исполнение обычного метода - непосредственное исполнение и последующее возвращение результата его работы.
            return get_method.method->body->Execute(method_closure, context);
        }
    }

    [[nodiscard]] std::string ClassInstance::GetClassName() const
    {
        return my_class_.GetName();
    }

    [[nodiscard]] bool ClassInstance::IsSuccessorOf(const std::string& test_my_parent) const
    {
        return my_class_.IsSuccessorOf(test_my_parent);
    }
    
    [[nodiscard]] bool ClassInstance::IsSuccessorOf(const Class& test_my_parent) const
    {
        return my_class_.IsSuccessorOf(test_my_parent);
    }

    Class::Class(std::string name, std::vector<Method> methods, std::vector<const Class*> parents) :
        my_name_(move(name)), my_id_(parse::TypeIdentificator::GetNewTypeId())
    {
        // Заполняем вектор ссылок на предков.
        for (const Class* scan_parent : parents)
            parents_.push_back(*scan_parent);
        // Заполняем таблицу виртуальных методов образуемого класса, выполняя при этом некоторую проверку корректности переданных нам методов.
        for (Method& method : methods)
        {
            auto eq_name_pair = virtual_method_table_.equal_range(method.name);
            for (auto scan_method_it = eq_name_pair.first; scan_method_it != eq_name_pair.second; ++scan_method_it)
            { // Проверка на отсутствие в составе класса метода с аналогичной сигнатурой (именем и количеством параметров).
                if (scan_method_it->second.formal_params.size() == method.formal_params.size())
                    throw ParseError(ThrowMessageNumber::THRM_AMBIGUOUS_OVERLOAD);
            }

            virtual_method_table_.insert({method.name, move(method)});
        }
    }

    Class::GetMethodRet Class::GetMethod(const std::string& name, int args_count, const std::string& parent_name) const
    {
        GetMethodRet found_method;
        bool is_parent_class_found = false;
        bool is_method_found = TraverseParents([&](const Class& scan_parent) -> bool
            {
                if (!parent_name.empty())
                {
                    if (scan_parent.GetName() == parent_name)
                        is_parent_class_found = true;
                    else
                        return false; // Если parent_name не пуст, то нас удовлетворят только методы именно этого класса.
                }

                auto name_range_pair = scan_parent.virtual_method_table_.equal_range(name);
                for (auto test_method_it = name_range_pair.first; test_method_it != name_range_pair.second; ++test_method_it)
                {
                    const runtime::Method& test_method = test_method_it->second;
                    // Если args_count < 0, проверка на соответствие количеству формальных параметров не проводится.
                    if (args_count < 0 || args_count == static_cast<int>(test_method.formal_params.size()))
                    { // Нужный нам метод успешно найден - он имеет нужное имя, требуемое число параметров и, если указано,
                        // принадлежит указанному классу.
                        found_method = &test_method;
                        return true;
                    }
                }
                // В этом предке требуемого метода нет, переходим к обследованию других классов - либо "братских" классов (классов на
                // том же уровне родственной иерархии), либо его собственных предков.
                return false;
            });
        // Возвращаем результат поиска либо в виде дескриптора найденного метода, либо в виде кода ошибки.
        if (!parent_name.empty() && !is_parent_class_found)
            return ThrowMessageNumber::THRM_QUALIFIER_NOT_ANCESTOR;
        else if (is_method_found)
            return found_method;
        else
            return ThrowMessageNumber::THRM_METHOD_NOT_FOUND;
    }

    std::vector<std::pair<std::string, size_t>> Class::GetMethodsDesc() const
    {
        // Так как обход родственного дерева производится от потомков к предкам, то методы потомков перекрывают соответствующие им методы
        // предков (с совпадающими сигнатурами).
        std::vector<std::pair<std::string, size_t>> result;
        TraverseParents([&result](const Class& scan_parent) -> bool
            {
                for (auto& method_table_pair : scan_parent.virtual_method_table_)
                {
                    std::pair<std::string, size_t> method_def_pair{method_table_pair.second.name, method_table_pair.second.formal_params.size()};
                    if (std::find(result.begin(), result.end(), method_def_pair) == result.end())
                        // Ранее метода с такой сигнатурой ещё не встречалось.
                        result.emplace_back(move(method_def_pair));
                }
                return false;   // Здесь мы всегда обходим полное дерево родства данного класса.
            });
        return result;
    }

    [[nodiscard]] const std::string& Class::GetName() const
    {
        return my_name_;
    }

    void Class::Print(ostream& os, [[maybe_unused]] Context& context)
    {
        os << "Class " << my_name_;
    }

    bool Class::IsSuccessorOf(const std::string& test_my_parent) const
    {
        return TraverseParents([&test_my_parent](const Class& scan_parent) -> bool
            {
                return scan_parent.GetName() == test_my_parent;
            });
    }

    bool Class::IsSuccessorOf(const Class& test_my_parent) const
    {
        return TraverseParents([&test_my_parent](const Class& scan_parent) -> bool
            {
                return &scan_parent == &test_my_parent;
            });
    }

    bool Class::TraverseParents(std::function<bool(const Class&)> handle_parent_func) const
    { // Фунция выполняет обход дерева предков "в ширину". При этом "верхние" узлы (соответствующие потомкам) обходятся ранее,
      // чем узлы "нижние" (соответствующие предкам). Таким образом, потомки имеют приоритет перед предками и посещаются первыми
      // (ранее своих предков). На одном уровне родства обход производится в порядке следования в массиве parents_.
        std::queue<ParentRefType> nodes_queue;
        // Начинаем обход дерева с нас самих. Собственный объект будет корнем дерева предков.
        nodes_queue.push(std::cref(*this));

        while (!nodes_queue.empty())
        {
            ParentRefType current_parent_ref = nodes_queue.front(); // Текущий анализируемый (просматриваемый) класс.
            nodes_queue.pop();
            if (handle_parent_func(current_parent_ref))
                return true; // Если обработчик вернул "ИСТИНУ", то он всем уже удовлетворён и обход предков прекращаем.
            
            // Ставим в конец очереди просмотра всех предков текущего просматриваемого класса, хранящихся в его поле parents_.
            for (ParentRefType up_parent_ref : current_parent_ref.get().parents_)
                nodes_queue.push(up_parent_ref);
        }
        return false;
    }

    const void* Number::GetPtr() const
    {
        if (std::holds_alternative<int>(value_))
            return &std::get<int>(value_);
        else if (std::holds_alternative<double>(value_))
            return &std::get<double>(value_);
        else
            return nullptr;
    }

    size_t Number::SizeOf() const
    {
        if (std::holds_alternative<int>(value_))
            return sizeof(int);
        else if (std::holds_alternative<double>(value_))
            return sizeof(double);
        else
            return 0;
    }

    void Number::Print(std::ostream& os, [[maybe_unused]] Context& context)
    {
        if (IsInt())
            os << GetIntValue();
        else if (IsDouble())
            os << GetDoubleValue();
    }

    void Bool::Print(std::ostream& os, [[maybe_unused]] Context& context)
    {
        os << (GetValue() ? "True"sv : "False"sv);
    }
    
    Method::Method(std::string p_name, std::vector<std::string> p_formal_params, std::unique_ptr<Executable> p_body, bool p_is_coroutine) :
        name(move(p_name)),
        formal_params(move(p_formal_params)),
        body(move(p_body)),
        is_coroutine(p_is_coroutine)
    {
        TuneBodyReference();
    }

    Method::Method(Method&& other) noexcept :
        name(move(other.name)),
        formal_params(move(other.formal_params)),
        body(move(other.body)),
        is_coroutine(other.is_coroutine)
    {
        TuneBodyReference();
    }
        
    Method& Method::operator=(Method&& other) noexcept
    {
        if (this != &other)
        {
            name = move(other.name);
            formal_params = move(other.formal_params);
            body = move(other.body);
            is_coroutine = other.is_coroutine;
            TuneBodyReference();
        }
        return *this;
    }

    void Method::TuneBodyReference()
    {
        if (ast::MethodBody* method_body = dynamic_cast<ast::MethodBody*>(body.get()))
        {
            method_body->SetSentinelInfoData(true, &name);
            method_body->SetSentinelInfoData(false, &name);
        }
    }

    WorkflowPosition::WorkPosType WorkflowPosition::GetType() const
    {
        if (std::holds_alternative<MethodWorkflowPosData>(pos_data_))
            return WorkPosType::WORK_POS_METHOD;
        else if (std::holds_alternative<CompoundWorkflowPosData>(pos_data_))
            return WorkPosType::WORK_POS_COMPOUND;
        else if (std::holds_alternative<IfElseWorkflowPosData>(pos_data_))
            return WorkPosType::WORK_POS_IF_ELSE;
        else if (std::holds_alternative<WhileWorkflowPosData>(pos_data_))
            return WorkPosType::WORK_POS_WHILE;
        else if (std::holds_alternative<CoYieldWorkflowPosData>(pos_data_))
            return WorkPosType::WORK_POS_CO_YIELD;
        else if (std::holds_alternative<TryExceptWorkflowPosData>(pos_data_))
            return WorkPosType::WORK_POS_TRY_EXCEPT;

        return WorkPosType::WORK_POS_UNKNOWN;
    }

    WorkflowPosition* WorkflowStackSaver::PushBack(WorkflowPosition new_workflow_position)
    {
        workflow_data_.push_back(new_workflow_position);
        current_workflow_index_ = static_cast<int>(workflow_data_.size()) - 1;
        return Current();
    }

    WorkflowPosition WorkflowStackSaver::PopBack(WorkflowPosition::WorkPosType find_pos_type)
    {
        while (!workflow_data_.empty())
        {
            WorkflowPosition top_workflow_pos = std::move(workflow_data_.back());
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

    WorkflowPosition* WorkflowStackSaver::Current()
    {
        if (current_workflow_index_ >= 0 && current_workflow_index_ < static_cast<int>(workflow_data_.size()))
        {
            decltype(workflow_data_)::iterator workflow_begin_it = workflow_data_.begin();
            std::advance(workflow_begin_it, current_workflow_index_);
            return &(*workflow_begin_it);
        }
        else
        {
            return nullptr;
        }
    }

    WorkflowPosition* WorkflowStackSaver::SetIndex(int new_index)
    {
        current_workflow_index_ = new_index;
        CorrectCurrentIndex();
        return Current();
    }

    WorkflowPosition* WorkflowStackSaver::Advance(int dist)
    {  // Сдвиг индекса текущей позиции потока управления на dist элементов (вперед или назад).
        current_workflow_index_ += dist;
        CorrectCurrentIndex();
        return Current();
    }

    WorkflowPosition* WorkflowStackSaver::Back()
    {
        if (!workflow_data_.empty())
            return &(workflow_data_.back());
        else
            return nullptr;
    }

    void WorkflowStackSaver::Clear()
    {
        workflow_data_.clear();
        current_workflow_index_ = 0;
    }

    void WorkflowStackSaver::CorrectCurrentIndex()
    {
        if (current_workflow_index_ < -1)
            current_workflow_index_ = -1;
        if (current_workflow_index_ > static_cast<int>(workflow_data_.size()))
            current_workflow_index_ = static_cast<int>(workflow_data_.size());
    }

    void WorkflowStackSaver::Print(std::ostream& os, [[maybe_unused]] Context& context)
    {
        if (Current())
            os << "Поток исполнения в " << static_cast<int>(Current()->GetType());
        else
            os << "Поток исполнения не зафиксирован";
    }

    // Функция обобщённого сравнения строк на равенство и "меньше" с учётом их кодировок и представления.
    static bool StrCompareOp(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context, bool is_less_op)
    {
        const runtime::String* lhs_str = lhs.TryAs<String>();
        const runtime::String* rhs_str = rhs.TryAs<String>();
        // Режим сравнения строк выбирается первым аргументом (lhs) сравнения.
        ObjectHolder cnv_rhs;
        if (lhs_str->encoding != rhs_str->encoding)
        { // Если кодировки сравниваемых строк не совпадают, приведём вторую строку к кодировке первой.
            cnv_rhs = StringOpsInstance::ConvertTranscodeTo(rhs, context, lhs_str->encoding);
            rhs_str = cnv_rhs.TryAs<String>();
        }

        int compare_result = 0;
        if (lhs_str->encoding == UTF_8_ENCODING)
        { // Сравнение строк в представлении UTF-8 выполняется всегда путём прямого сопоставления индивидуальных
          // Юникодов входящих в них многобайтовых символов.
            if (!is_less_op)
            { // При сравнении UTF-8-строк на точное равенство можно сравнить такие строки ускоренно, без выделения и
              // анализа отдельных кодов каждого из составляющих их символов.
                return (lhs_str->utf8_map.begin_map.size() == rhs_str->utf8_map.begin_map.size() &&
                        lhs_str->GetValue() == rhs_str->GetValue());
            }
            compare_result = CompareUTF8(lhs_str->GetValue(), rhs_str->GetValue());
        }
        else
        { // Режим сравнения "узких" однобайтовых строк определяется установками для первого операнда сравнения (lhs).
            CompareCollateMode compare_mode
            {
                .upcase_table = lhs_str->GetUpcaseTable(),
                .collate = lhs_str->GetCollate(),
                .is_use_collate = lhs_str->is_use_collate,
                .is_equal_collate = lhs_str->is_equal_collate,
                .is_case_indep_compare = lhs_str->is_case_indep_compare
            };
            compare_result = CompareCollate(lhs_str->GetValue(), rhs_str->GetValue(), compare_mode);
        }
        if (is_less_op) // Это сравнение на "меньше" (на <).
            return compare_result < 0;
        else // Это сравнение на равенство (на ==).
            return compare_result == 0;
    }

    bool Equal(const ObjectHolder& lhs, const ObjectHolder& rhs, [[maybe_unused]] Context& context)
    {
        if (lhs.TryAs<Number>() && rhs.TryAs<Number>())
            return (*lhs.TryAs<Number>()) == (*rhs.TryAs<Number>());

        if (lhs.TryAs<String>() && rhs.TryAs<String>())
            return StrCompareOp(lhs, rhs, context, false);

        if (lhs.TryAs<Bool>() && rhs.TryAs<Bool>())
            return lhs.TryAs<Bool>()->GetValue() == rhs.TryAs<Bool>()->GetValue();

        if (!lhs && !rhs)
            return true;    // Значения None считаются равными.

        if (CommonClassInstance* lhs_inst_ptr = lhs.TryAs<ClassInstance>())
            if (lhs_inst_ptr->HasMethod(EQUAL_CMP_METHOD, 1))
                return IsTrue(lhs_inst_ptr->Call(EQUAL_CMP_METHOD, {rhs}, context));

        ThrowRuntimeError(context, ThrowMessageNumber::THRM_IMPOSSIBLE_COMPARE_EQUAL);
    }

    bool Less(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context)
    {
        if (lhs.TryAs<Number>() && rhs.TryAs<Number>())
            return (*lhs.TryAs<Number>()) < (*rhs.TryAs<Number>());

        if (lhs.TryAs<String>() && rhs.TryAs<String>())
            return StrCompareOp(lhs, rhs, context, true);

        if (lhs.TryAs<Bool>() && rhs.TryAs<Bool>())
            return lhs.TryAs<Bool>()->GetValue() < rhs.TryAs<Bool>()->GetValue();

        if (CommonClassInstance * lhs_inst_ptr = lhs.TryAs<ClassInstance>())
            if (lhs_inst_ptr->HasMethod(LESS_CMP_METHOD, 1))
                return IsTrue(lhs_inst_ptr->Call(LESS_CMP_METHOD, {rhs}, context));

        ThrowRuntimeError(context, ThrowMessageNumber::THRM_IMPOSSIBLE_COMPARE_LESS);
    }

    bool NotEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context)
    {
        return !Equal(lhs, rhs, context);
    }

    bool Greater(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context)
    {
        return !Less(lhs, rhs, context) && !Equal(lhs, rhs, context);
    }

    bool LessOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context)
    {
        return Less(lhs, rhs, context) || Equal(lhs, rhs, context);
    }

    bool GreaterOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context)
    {
        return !Less(lhs, rhs, context);
    }

    SimpleContext::SimpleContext(ostream& output, LinkageFunction external_link)
        : output_(output), external_link_(move(external_link))
    {
        // Формируем стартовый набор опций контекста по умолчанию.
        opt_data_.emplace(OptionType::CONTEXT_OPT_DESTRUCT_AT_FINISH, true);
        opt_data_.emplace(OptionType::CONTEXT_OPT_SKIP_DECLARATIVE, true);
        opt_data_.emplace(OptionType::CONTEXT_OPT_SKIP_CALL_FRAME, true);
        // Описательные (неисполнительные) отладочные звонки в умолчательном режиме ограничиваться не будут (если они, конечно,
        // включены своими специальными опциями).
        opt_data_.emplace(OptionType::CONTEXT_OPT_ONCE_ANY_CALL, false);
        opt_data_.emplace(OptionType::CONTEXT_OPT_ONCE_NONEXEC_CALL, false);
        opt_data_.emplace(OptionType::CONTEXT_OPT_ONCE_EXEC_CALL, true);
        // Режимы обработки бряков по умолчанию приняты следующие: не более одного звонка на строку, пропускать декларации
        // (на них бряки не активируются), рамочные узлы обрабатываются обычным образом, предпочтение отдаётся исполняющимся
        // инструкциям.
        opt_data_.emplace(OptionType::CONTEXT_OPT_ONCE_BREAKPOINT_CALL, true);
        opt_data_.emplace(OptionType::CONTEXT_OPT_BREAK_SKIP_DECLARATIVE, true);
        opt_data_.emplace(OptionType::CONTEXT_OPT_BREAK_SKIP_CALL_FRAME, false);
        opt_data_.emplace(OptionType::CONTEXT_OPT_BREAK_PREFER_EXEC, true);
    }

    #ifndef MYTHON_UNITHREAD
        // Вариант с синхронизацией при параллельном доступе.
        SimpleContext::SimpleContext(SimpleContext&& other) noexcept :
            output_(other.output_), external_link_(move(other.external_link_)),
            is_terminate_{other.is_terminate_.exchange(true)},
            opt_data_(move(other.opt_data_))
        {}
    #else
        // Вариант без применения механизмов многопоточности.
        SimpleContext::SimpleContext(SimpleContext&& other) noexcept :
            output_(other.output_), external_link_(move(other.external_link_)),
            is_terminate_{other.is_terminate_}, opt_data_(move(other.opt_data_))
        {
            other.is_terminate_ = true;
        }
    #endif

    LinkageValue SimpleContext::GetOption(OptionType ask_option)
    {
        #ifndef MYTHON_UNITHREAD
            std::lock_guard lg(opt_mutex_);
        #endif

        if (opt_data_.contains(ask_option))
            return opt_data_.at(ask_option);
        else
            return {};
    }

    bool SimpleContext::SetOption(OptionType set_option, const LinkageValue& option_value)
    {
        #ifndef MYTHON_UNITHREAD
            std::lock_guard lg(opt_mutex_);
        #endif

        bool result = opt_data_.contains(set_option);
        opt_data_.emplace(set_option, option_value);
        return result;
    }

    std::string CommandGenusToString(CommandGenus cmd_genus)
    {
        switch (cmd_genus)
        {
        case CommandGenus::CMD_GENUS_COMMON:
            // Обыкновенная инструкция общего назначения.
            return "Инструкция";
        case CommandGenus::CMD_GENUS_CALL_METHOD:
            // Команда вызова метода или свободной функции.
            return "Вызов метода";
        case CommandGenus::CMD_GENUS_RETURN_FROM_METHOD:
            return "Возврат из метода";
        case CommandGenus::CMD_GENUS_DECLARATIVE:
            // Организующая инструкция (узел АСД) без содержательных действий при исполнении.
            return "Декларация";
        case CommandGenus::CMD_GENUS_PRE_FIRST_METHOD_STMT:
            // Пседвоинструкция-маркёр начала метода или функции (расположена перед первой их действительной командой).
            return "Начало метода";
        case CommandGenus::CMD_GENUS_AFTER_LAST_METHOD_STMT:
            // Маркёрная псевдоинструкция, размещённая в его конце (за последней его действительной командой).
            return "Конец метода";
        case CommandGenus::CMD_GENUS_INITIALIZE:
            return "Инициализация";
        default:
            return "Другое";
        };
    }

    std::ostream& operator<<(std::ostream& ostr, CommandGenus cmd_genus)
    {
        static constexpr int COMMAND_GENUS_WIDTH = 20;

        std::string cmd_genus_str = CommandGenusToString(cmd_genus);
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        std::wstring wide_cmd_genus_str = converter.from_bytes(cmd_genus_str);

        int symb_delta = COMMAND_GENUS_WIDTH - static_cast<int>(wide_cmd_genus_str.size());
        if (symb_delta > 0)
            wide_cmd_genus_str += std::wstring(symb_delta, ' ');
        else if (symb_delta < 0)
            wide_cmd_genus_str = wide_cmd_genus_str.substr(0, COMMAND_GENUS_WIDTH);

        ostr << converter.to_bytes(wide_cmd_genus_str);
        return ostr;
    }
}  // namespace runtime

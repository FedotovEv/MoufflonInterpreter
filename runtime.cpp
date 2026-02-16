
#include "runtime.h"
#include "parse.h"

#include <cassert>
#include <optional>
#include <sstream>

using namespace std;

namespace runtime
{
    [[noreturn]] void ThrowRuntimeError(runtime::Executable* exec_obj_ptr, const string& except_text)
    {
        string command_desc = to_string(exec_obj_ptr->GetCommandDesc().module_id) + "("s +
            to_string(exec_obj_ptr->GetCommandDesc().module_string_number) + "):"s;
        throw runtime_error(command_desc + except_text);
    }

    [[noreturn]] void ThrowRuntimeError(runtime::Executable* exec_obj_ptr, ThrowMessageNumber msg_num)
    {
        string command_desc = to_string(exec_obj_ptr->GetCommandDesc().module_id) + "("s +
            to_string(exec_obj_ptr->GetCommandDesc().module_string_number) + "):"s;
        throw runtime_error(command_desc + ThrowMessages::GetThrowText(msg_num));
    }

    [[noreturn]] void RethrowRuntimeError(runtime::Executable* exec_obj_ptr, runtime_error & orig_runtime_error)
    {
        string command_desc = to_string(exec_obj_ptr->GetCommandDesc().module_id) + "("s +
            to_string(exec_obj_ptr->GetCommandDesc().module_string_number) + "):"s;
        throw runtime_error(command_desc + orig_runtime_error.what());
    }

    [[noreturn]] void ThrowRuntimeError(Context& context, const string& except_text)
    {
        string command_desc = to_string(context.GetLastCommandDesc().module_id) + "("s +
            to_string(context.GetLastCommandDesc().module_string_number) + "):"s;
        throw runtime_error(command_desc + except_text);
    }

    [[noreturn]] void ThrowRuntimeError(Context& context, ThrowMessageNumber msg_num)
    {
        string command_desc = to_string(context.GetLastCommandDesc().module_id) + "("s +
            to_string(context.GetLastCommandDesc().module_string_number) + "):"s;
        throw runtime_error(command_desc + ThrowMessages::GetThrowText(msg_num));
    }

    [[noreturn]] void RethrowRuntimeError(Context& context, runtime_error& orig_runtime_error)
    {
        string command_desc = to_string(context.GetLastCommandDesc().module_id) + "("s +
            to_string(context.GetLastCommandDesc().module_string_number) + "):"s;
        throw runtime_error(command_desc + orig_runtime_error.what());
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
        return ObjectHolder(std::shared_ptr<Object>(&object, empty_deleter));
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

    Object* ObjectHolder::Get() const
    {
        return data_.get();
    }

    void ObjectHolder::ModifyData(const ObjectHolder& object_holder)
    {
        data_ = object_holder.data_;
    }

    ObjectHolder::operator bool() const
    {
        return Get() != nullptr;
    }

    bool ObjectHolder::IsOwning() const
    {
        return IsOwning(data_);
    }

    bool ObjectHolder::IsOwning(const std::shared_ptr<Object>& test_ptr) const
    {
        if (!test_ptr)
            return false;
        if (auto data_del_p = std::get_deleter<void(*)(Object*)>(test_ptr))
            return *data_del_p != empty_deleter;
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

    void ClassInstance::Print(std::ostream& os, Context& context)
    {
        if (HasMethod(STR_FUNCTION_METHOD, 0))
            Call(STR_FUNCTION_METHOD, {}, context)->Print(os, context);
        else
            os << this;
    }

    bool ClassInstance::HasMethod(const std::string& method_name, size_t argument_count) const
    {
        if (const Method* method_ptr = my_class_.GetMethod(method_name, static_cast<int>(argument_count)))
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
    {
        dummy_statement_->SetCommandGenus(runtime::CommandGenus::CMD_GENUS_CALL_METHOD);
    }

    ObjectHolder ClassInstance::Call(const std::string& method_name,
                                     const std::vector<ObjectHolder>& actual_args,
                                     Context& context, const std::string& parent_name)
    {
        Class::GetMethodRet get_method = my_class_.GetMethod(method_name, static_cast<int>(actual_args.size()), parent_name);
        if (!get_method || get_method.method->formal_params.size() != actual_args.size())
            ThrowRuntimeError(context, get_method.error);
    
        Closure method_closure;
        method_closure[SELF_FIELD_NAME] = ObjectHolder::Share(*this);
        auto actual_args_it = actual_args.begin();
        for (const string& formal_param_name : get_method.method->formal_params)
            method_closure[formal_param_name] = *actual_args_it++;
    
        dummy_statement_->info_data_ptr = &method_name;
        PrepareExecute(dummy_statement_.get(), method_closure, context);

        if (get_method.method->is_coroutine)
            // Запуск сопрограммы. Она пока только готовится к запуску и будет находиться в приостановленном состоянии.
            return ObjectHolder::Own(move(CoroutineInstance(this, get_method.method, method_closure)));
        else // Исполнение обычного метода.
            return get_method.method->body->Execute(method_closure, context);
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

    Class::Class(std::string name, std::vector<Method> methods, const Class* parent) :
        my_name_(move(name)), parent_(*parent)
    {
        for (Method& method : methods)
        {
            auto eq_name_pair = virtual_method_table_.equal_range(method.name);
            for (auto scan_method_it = eq_name_pair.first; scan_method_it != eq_name_pair.second; ++scan_method_it)
            { // Проверка на отсутствие в составе класса метода с аналогичной сигнатурой (именем и количеством параметров).
                if (scan_method_it->second.formal_params.size() == method.formal_params.size())
                    throw ParseError(runtime::ThrowMessageNumber::THRM_AMBIGUOUS_OVERLOAD);
            }

            virtual_method_table_.insert({method.name, move(method)});
            //virtual_method_table_[method.name] = move(method);
        }
    }

    Class::GetMethodRet Class::GetMethod(const std::string& name, int args_count, const std::string& parent_name) const
    {
        const Class* current_class = this;
        if (!parent_name.empty())
        { // Если parent_name не пуст, ищем среди наших предков класс с таким именем.
            while (current_class)
            {
                if (current_class->GetName() == parent_name)
                    break;
                current_class = &current_class->parent_;
            }
        }
        if (!current_class)
            return ThrowMessageNumber::THRM_QUALIFIER_NOT_ANCESTOR;

        while (current_class)
        {
            if (current_class->virtual_method_table_.count(name))
            {
                auto name_range_pair = current_class->virtual_method_table_.equal_range(name);
                for (auto test_method_it = name_range_pair.first; test_method_it != name_range_pair.second; ++test_method_it)
                {
                    const runtime::Method& test_method = test_method_it->second;
                    if (args_count < 0)
                        return &test_method; // Проверка на соответствие количеству формальных параметров в данном случае не проводится.
                    if (args_count == test_method.formal_params.size())
                        return &test_method; // Метод test_method имеет точно затребованное количество параметров.
                }
            }
            
            current_class = &current_class->parent_;
        }
        return ThrowMessageNumber::THRM_METHOD_NOT_FOUND;
    }

    std::vector<std::pair<std::string, size_t>> Class::GetMethodsDesc() const
    {
        std::unordered_map<std::string, size_t> intermediate_result;
        const Class* current_class = this;
        while (current_class)
        {
            for (auto& method_table_pair : current_class->virtual_method_table_)
                intermediate_result[method_table_pair.second.name] = method_table_pair.second.formal_params.size();
            current_class = &current_class->parent_;
        }

        std::vector<std::pair<std::string, size_t>> result;
        for (auto& current_method_pair : intermediate_result)
            result.emplace_back(current_method_pair.first, current_method_pair.second);
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
        const Class* test_class = this;
        while (test_class)
        {
            if (test_class->GetName() == test_my_parent)
                return true;
            test_class = &parent_;
        }

        return false;
    }

    bool Class::IsSuccessorOf(const Class& test_my_parent) const
    {
        const Class* test_class = this;
        while (test_class)
        {
            if (test_class == &test_my_parent)
                return true;
            test_class = &parent_;
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

    bool Equal(const ObjectHolder& lhs, const ObjectHolder& rhs, [[maybe_unused]] Context& context)
    {
        if (lhs.TryAs<Number>() && rhs.TryAs<Number>())
            return (*lhs.TryAs<Number>()) == (*rhs.TryAs<Number>());

        if (lhs.TryAs<String>() && rhs.TryAs<String>())
            return lhs.TryAs<String>()->GetValue() == rhs.TryAs<String>()->GetValue();

        if (lhs.TryAs<Bool>() && rhs.TryAs<Bool>())
            return lhs.TryAs<Bool>()->GetValue() == rhs.TryAs<Bool>()->GetValue();

        if (!lhs && !rhs)
            return true;

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
            return lhs.TryAs<String>()->GetValue() < rhs.TryAs<String>()->GetValue();

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
}  // namespace runtime

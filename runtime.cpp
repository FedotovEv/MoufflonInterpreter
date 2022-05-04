#include "runtime.h"

#include <cassert>
#include <optional>
#include <sstream>

using namespace std;

namespace runtime
{
    ObjectHolder::ObjectHolder(std::shared_ptr<Object> data)
        : data_(std::move(data)) {
    }

    void ObjectHolder::AssertIsValid() const {
        assert(data_ != nullptr);
    }

    ObjectHolder ObjectHolder::Share(Object& object) {
        // Возвращаем невладеющий shared_ptr (его удалитель ничего не делает)
        return ObjectHolder(std::shared_ptr<Object>(&object, [](auto* /*p*/) { /* Не делает ничего. Абсолютно ничего */ }));
    }

    ObjectHolder ObjectHolder::None() {
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

    ObjectHolder::operator bool() const
    {
        return Get() != nullptr;
    }

    bool IsTrue(const ObjectHolder& object)
    {
        if (Number* number_ptr = object.TryAs<Number>())
            return number_ptr->GetValue();
    
        if (String* string_ptr = object.TryAs<String>())
            return string_ptr->GetValue().size();

        if (Bool* bool_ptr = object.TryAs<Bool>())
            return bool_ptr->GetValue();

        return false;
    }

    void ClassInstance::Print(std::ostream& os, Context& context)
    {
        if (HasMethod("__str__", 0))
            Call("__str__", {}, context)->Print(os, context);
        else
            os << this;
    }

    bool ClassInstance::HasMethod(const std::string& method_name, size_t argument_count) const
    {
        if (const Method* method_ptr = my_class_.GetMethod(method_name))
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
                                     Context& context)
    {
        const Method* method_ptr = my_class_.GetMethod(method_name);
        if (!method_ptr ||
            method_ptr->formal_params.size() != actual_args.size())
            throw std::runtime_error("Метод не найден"s);
    
        Closure method_closure;
        method_closure["self"s] = ObjectHolder::Share(*this);
        auto actual_args_it = actual_args.begin();
        for (const string& formal_param_name : method_ptr->formal_params)
            method_closure[formal_param_name] = *actual_args_it++;
    
        return method_ptr->body->Execute(method_closure, context);
    }

    [[nodiscard]] std::string ClassInstance::GetClassName() const
    {
        return my_class_.GetName();
    }

    Class::Class(std::string name, std::vector<Method> methods, const Class* parent) :
        my_name_(move(name)), parent_(*parent)
    {
        for (Method& method : methods)
            virtual_method_table_[method.name] = move(method);
    }

    const Method* Class::GetMethod(const std::string& name) const
    {
        const Class* current_class = this;
        while (current_class)
        {
            if (current_class->virtual_method_table_.count(name))
                return &(current_class->virtual_method_table_.at(name));
            else
                current_class = &current_class->parent_;
        }
        return nullptr;    
    }

    [[nodiscard]] const std::string& Class::GetName() const
    {
        return my_name_;
    }

    void Class::Print(ostream& os, [[maybe_unused]] Context& context)
    {
        os << "Class " << my_name_;
    }

    void Bool::Print(std::ostream& os, [[maybe_unused]] Context& context)
    {
        os << (GetValue() ? "True"sv : "False"sv);
    }

    bool Equal(const ObjectHolder& lhs, const ObjectHolder& rhs, [[maybe_unused]] Context& context)
    {
        if (lhs.TryAs<Number>() && rhs.TryAs<Number>())
            return lhs.TryAs<Number>()->GetValue() == rhs.TryAs<Number>()->GetValue();

        if (lhs.TryAs<String>() && rhs.TryAs<String>())
            return lhs.TryAs<String>()->GetValue() == rhs.TryAs<String>()->GetValue();

        if (lhs.TryAs<Bool>() && rhs.TryAs<Bool>())
            return lhs.TryAs<Bool>()->GetValue() == rhs.TryAs<Bool>()->GetValue();

        if (!lhs && !rhs)
            return true;

        if (ClassInstance* lhs_inst_ptr = lhs.TryAs<ClassInstance>())
            if (lhs_inst_ptr->HasMethod("__eq__", 1))
                return IsTrue(lhs_inst_ptr->Call("__eq__", {rhs}, context));

        throw std::runtime_error("Невозможно сравнить объекты на равенство"s);
    }

    bool Less(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context)
    {
        if (lhs.TryAs<Number>() && rhs.TryAs<Number>())
            return lhs.TryAs<Number>()->GetValue() < rhs.TryAs<Number>()->GetValue();

        if (lhs.TryAs<String>() && rhs.TryAs<String>())
            return lhs.TryAs<String>()->GetValue() < rhs.TryAs<String>()->GetValue();

        if (lhs.TryAs<Bool>() && rhs.TryAs<Bool>())
            return lhs.TryAs<Bool>()->GetValue() < rhs.TryAs<Bool>()->GetValue();

        if (ClassInstance * lhs_inst_ptr = lhs.TryAs<ClassInstance>())
            if (lhs_inst_ptr->HasMethod("__lt__", 1))
                return IsTrue(lhs_inst_ptr->Call("__lt__", {rhs}, context));

        throw std::runtime_error("Невозможно сравнить объекты на \"меньше\""s);
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

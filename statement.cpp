#include "statement.h"

#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace std;

namespace ast
{
using runtime::Closure;
using runtime::Context;
using runtime::ObjectHolder;

namespace
{
    const string ADD_METHOD = "__add__"s;
    const string INIT_METHOD = "__init__"s;
    const string EXTERNAL_LINK_CLASS_NAME = "__external"s;
}  // namespace

ObjectHolder Assignment::Execute(Closure& closure, Context& context)
{
    return closure[var_] = rv_->Execute(closure, context);
}

Assignment::Assignment(std::string var, std::unique_ptr<Statement> rv) : var_(move(var)), rv_(move(rv))
{}

VariableValue::VariableValue(const std::string& var_name)
{
    dotted_ids_.push_back(var_name);
}

VariableValue::VariableValue(std::vector<std::string> dotted_ids) : dotted_ids_(move(dotted_ids))
{}

ObjectHolder VariableValue::Execute(Closure& closure, [[maybe_unused]] Context& context)
{
    size_t i = 1;
    Closure* cur_closure_ptr = &closure;
    runtime::ClassInstance* cur_class_instance_ptr = nullptr;
    for (const string id_name : dotted_ids_)
    {
        if (!cur_closure_ptr->count(id_name))
        {
            throw runtime_error("Переменная не найдена");
        }
        if (i++ < dotted_ids_.size())
        {
            cur_class_instance_ptr = cur_closure_ptr->at(id_name).TryAs<runtime::ClassInstance>();
            cur_closure_ptr = &(cur_class_instance_ptr->Fields());
        }
        else
        {
            if (cur_class_instance_ptr && cur_class_instance_ptr->GetClassName() == EXTERNAL_LINK_CLASS_NAME &&
                context.GetExternalLinkage() && id_name.size())
            {  // Вызов звонковой функции при чтении полей объекта "__external"
                variant<int, string> external_result = context.GetExternalLinkage()(runtime::LinkCallReason::CALL_REASON_READ_FIELD,
                                                                                    id_name, {});
                if (holds_alternative<int>(external_result))
                    return runtime::ObjectHolder::Own(runtime::ValueObject(get<int>(external_result)));
                else if (holds_alternative<string>(external_result))
                    return runtime::ObjectHolder::Own(runtime::ValueObject(get<string>(external_result)));
            }
            else
            {
                return cur_closure_ptr->at(id_name);
            }
        }
    }
    return {};
}

unique_ptr<Print> Print::Variable(const std::string& name)
{
    Print* result = new Print();
    result->name_ = name;
    return unique_ptr<Print>(result);
}

Print::Print(unique_ptr<Statement> argument)
{
    args_.push_back(move(argument));
}

Print::Print(vector<unique_ptr<Statement>> args) : args_(move(args))
{}

ObjectHolder Print::Execute(Closure& closure, Context& context)
{
    ObjectHolder result;
    if (name_.size())
    {
        result = closure.at(name_);
        if (result)
            result.Get()->Print(context.GetOutputStream(), context);
        context.GetOutputStream() << '\n';
    }
    else
    {
        size_t i = 1;
        for (auto& cur_statement_ptr : args_)
        {
            result = cur_statement_ptr->Execute(closure, context);
            if (result)
                result.Get()->Print(context.GetOutputStream(), context);
            else
                context.GetOutputStream() << "None";
            if (i++ < args_.size())
                context.GetOutputStream() << ' ';
        }
        context.GetOutputStream() << '\n';
    }
    return result;
}

MethodCall::MethodCall(unique_ptr<Statement> object, string method,
                       vector<std::unique_ptr<Statement>> args) :
                       object_(move(object)),
                       method_(move(method)),
                       args_(move(args))
{}

ObjectHolder MethodCall::Execute(Closure& closure, Context& context)
{
    ObjectHolder real_object = object_->Execute(closure, context);
    vector<ObjectHolder> real_args;
    for (auto& cur_arg_ptr : args_)
    { // Вычисляем истинные значения аргументов метода
        real_args.push_back(cur_arg_ptr->Execute(closure, context));
    }
    return real_object.TryAs<runtime::ClassInstance>()->Call(method_, real_args, context);
}

ObjectHolder Stringify::Execute(Closure& closure, Context& context)
{
    ObjectHolder object_hold = argument_->Execute(closure, context);
    ostringstream ostr;
    if (object_hold)
        object_hold->Print(ostr, context);
    else
        ostr << "None";
    return ObjectHolder::Own(runtime::String(ostr.str()));
}

// Поддерживается сложение:
 //  число + число
 //  строка + строка
 //  объект1 + объект2, если у объект1 - пользовательский класс с методом _add__(rhs)
 // В противном случае при вычислении выбрасывается runtime_error
ObjectHolder Add::Execute(Closure& closure, Context& context)
{
    runtime::ObjectHolder real_lhs(lhs_->Execute(closure, context));
    runtime::ObjectHolder real_rhs(rhs_->Execute(closure, context));

    if (real_lhs.TryAs<runtime::Number>() && real_rhs.TryAs<runtime::Number>())
    {
        int result = real_lhs.TryAs<runtime::Number>()->GetValue() + real_rhs.TryAs<runtime::Number>()->GetValue();
        return ObjectHolder::Own<runtime::Number>(result);
    }
    else if (real_lhs.TryAs<runtime::String>() && real_rhs.TryAs<runtime::String>())
    {
        string result = real_lhs.TryAs<runtime::String>()->GetValue() + real_rhs.TryAs<runtime::String>()->GetValue();
        return ObjectHolder::Own<runtime::String>(result);
    }
    else if (real_lhs.TryAs<runtime::ClassInstance>())
    {
        runtime::ClassInstance *lhs_class_ptr = real_lhs.TryAs<runtime::ClassInstance>();
        if (lhs_class_ptr->HasMethod(ADD_METHOD, 1))
            return lhs_class_ptr->Call(ADD_METHOD, { real_rhs }, context);
        else
            throw runtime_error("Невозможно выполнить сложение");
    }
    else
    {
        throw runtime_error("Невозможно выполнить сложение");
    }
}

// Поддерживается вычитание:
//  число - число
// Если lhs и rhs - не числа, выбрасывается исключение runtime_error
ObjectHolder Sub::Execute(Closure& closure, Context& context)
{
    runtime::ObjectHolder real_lhs(lhs_->Execute(closure, context));
    runtime::ObjectHolder real_rhs(rhs_->Execute(closure, context));

    if (real_lhs.TryAs<runtime::Number>() && real_rhs.TryAs<runtime::Number>())
    {
        int result = real_lhs.TryAs<runtime::Number>()->GetValue() - real_rhs.TryAs<runtime::Number>()->GetValue();
        return ObjectHolder::Own<runtime::Number>(result);
    }
    else
    {
        throw runtime_error("Невозможно выполнить вычитание");
    }
}

ObjectHolder Mult::Execute(Closure& closure, Context& context)
{
    runtime::ObjectHolder real_lhs(lhs_->Execute(closure, context));
    runtime::ObjectHolder real_rhs(rhs_->Execute(closure, context));

    if (real_lhs.TryAs<runtime::Number>() && real_rhs.TryAs<runtime::Number>())
    {
        int result = real_lhs.TryAs<runtime::Number>()->GetValue() * real_rhs.TryAs<runtime::Number>()->GetValue();
        return ObjectHolder::Own<runtime::Number>(result);
    }
    else
    {
        throw runtime_error("Невозможно выполнить умножение");
    }
}

ObjectHolder Div::Execute(Closure& closure, Context& context)
{
    runtime::ObjectHolder real_lhs(lhs_->Execute(closure, context));
    runtime::ObjectHolder real_rhs(rhs_->Execute(closure, context));

    if (real_lhs.TryAs<runtime::Number>() && real_rhs.TryAs<runtime::Number>())
    {
        int rhs_value = real_rhs.TryAs<runtime::Number>()->GetValue();
        if (!rhs_value)
            throw runtime_error("Деление на нуль");
        int result = real_lhs.TryAs<runtime::Number>()->GetValue() / rhs_value;
        return ObjectHolder::Own<runtime::Number>(result);
    }
    else
    {
        throw runtime_error("Невозможно выполнить деление");
    }
}

ObjectHolder Compound::Execute(Closure& closure, Context& context)
{
    for (auto& cur_statement_ptr : comp_body_)
        cur_statement_ptr->Execute(closure, context);
    return {};
}

ObjectHolder Return::Execute(Closure& closure, Context& context)
{
    throw ReturnResult(statement_->Execute(closure, context));
}

ClassDefinition::ClassDefinition(ObjectHolder cls) : cls_(move(cls))
{}

ObjectHolder ClassDefinition::Execute(Closure& closure, [[maybe_unused]] Context& context)
{
    closure[cls_.TryAs<runtime::Class>()->GetName()] = cls_;
    return cls_;
}

FieldAssignment::FieldAssignment(VariableValue object, std::string field_name,
                                 std::unique_ptr<Statement> rv) :
                                 object_(move(object)), field_name_(move(field_name)),
                                 rv_(move(rv))
{}

ObjectHolder FieldAssignment::Execute(Closure& closure, Context& context)
{ // Присваивает полю object.field_name значение выражения rv
    runtime::ClassInstance* target_object_ptr = nullptr;
    ObjectHolder target_object_holder(object_.Execute(closure, context));
    if (target_object_holder)
    {
        target_object_ptr = target_object_holder.TryAs<runtime::ClassInstance>();
        ObjectHolder value_holder = rv_->Execute(closure, context);
        if (target_object_ptr->GetClassName() == EXTERNAL_LINK_CLASS_NAME && 
            context.GetExternalLinkage() && field_name_.size() && value_holder)
            {
                ostringstream set_value_stream;
                value_holder->Print(set_value_stream, context);
                context.GetExternalLinkage()(runtime::LinkCallReason::CALL_REASON_WRITE_FIELD,
                                             field_name_, {set_value_stream.str()});
            }
        return (target_object_ptr->Fields())[field_name_] = move(value_holder);
    }
    return {};
}

IfElse::IfElse(std::unique_ptr<Statement> condition, std::unique_ptr<Statement> if_body,
               std::unique_ptr<Statement> else_body) :
               condition_(move(condition)), if_body_(move(if_body)), else_body_(move(else_body))
{}

ObjectHolder IfElse::Execute(Closure& closure, Context& context)
{
    if (runtime::IsTrue(condition_->Execute(closure, context)))
        return if_body_->Execute(closure, context);
    else
        if (else_body_)
            return else_body_->Execute(closure, context);
        else
            return {};
}

ObjectHolder Or::Execute(Closure& closure, Context& context)
{
    ObjectHolder real_lhs = lhs_->Execute(closure, context);
    // Значение аргумента rhs вычисляется, только если значение lhs
    // после приведения к Bool равно False
    if (!runtime::IsTrue(real_lhs))
    {
        ObjectHolder real_rhs = rhs_->Execute(closure, context);
        return ObjectHolder::Own(runtime::Bool(runtime::IsTrue(real_rhs)));
    }
    return ObjectHolder::Own(runtime::Bool(true));
}

ObjectHolder And::Execute(Closure& closure, Context& context)
{
    ObjectHolder real_lhs = lhs_->Execute(closure, context);
    // Значение аргумента rhs вычисляется, только если значение lhs
    // после приведения к Bool равно True
    if (runtime::IsTrue(real_lhs))
    {
        ObjectHolder real_rhs = rhs_->Execute(closure, context);
        return ObjectHolder::Own(runtime::Bool(runtime::IsTrue(real_rhs)));
    }
    return ObjectHolder::Own(runtime::Bool(false));
}

ObjectHolder Not::Execute(Closure& closure, Context& context)
{
    ObjectHolder real_arg = argument_->Execute(closure, context);
    return ObjectHolder::Own(runtime::Bool(!runtime::IsTrue(real_arg)));
}

Comparison::Comparison(Comparator cmp, unique_ptr<Statement> lhs, unique_ptr<Statement> rhs)
    : BinaryOperation(std::move(lhs), std::move(rhs)), cmp_(cmp)
{}

ObjectHolder Comparison::Execute(Closure& closure, Context& context)
{
    runtime::ObjectHolder real_lhs = lhs_->Execute(closure, context);
    runtime::ObjectHolder real_rhs = rhs_->Execute(closure, context);
    return ObjectHolder::Own(runtime::Bool(cmp_(real_lhs, real_rhs, context)));
}

NewInstance::NewInstance(const runtime::Class& class_, std::vector<std::unique_ptr<Statement>> args) :
                         new_class_instance_(class_), args_(move(args))
{}

NewInstance::NewInstance(const runtime::Class& class_) : new_class_instance_(class_)
{}

ObjectHolder NewInstance::Execute(Closure& closure, Context& context)
{
    if (new_class_instance_.HasMethod(INIT_METHOD, args_.size()))
    {
        vector<ObjectHolder> actual_args;
        for (auto& cur_param_ptr : args_)
            actual_args.push_back(cur_param_ptr->Execute(closure, context));
        new_class_instance_.Call(INIT_METHOD, actual_args, context);
    }
    return ObjectHolder::Share(new_class_instance_);
}

MethodBody::MethodBody(std::unique_ptr<Statement>&& body) : body_(move(body))
{}

ObjectHolder MethodBody::Execute(Closure& closure, Context& context)
{
    try
    {
        body_->Execute(closure, context);
    }
    catch (ReturnResult ret_result)
    {
        return ret_result.ret_result_;
    }
    return runtime::ObjectHolder::None();
}

}  // namespace ast

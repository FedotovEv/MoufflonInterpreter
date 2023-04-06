#include "statement.h"
#include "throw_messages.h"
#include "parse.h"
#include "debug_context.h"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cassert>

using namespace std;
using runtime::ThrowMessageNumber;
using runtime::ThrowMessages;
using runtime::Closure;
using runtime::Context;
using runtime::DebugContext;
using runtime::ObjectHolder;

void PrepareExecute(runtime::Executable* exec_obj_ptr, Closure& closure, Context& context)
{
    if (DebugContext* dbg_context = dynamic_cast<DebugContext*>(&context))
    { // Эти операции будут выполняться только при отладке, то есть если в качестве операнда context
      // передана переменная типа DebugContext.
        static bool is_wait_first_frame_command = false;
        // last_command - дескриптор последней корректной исполненной команды
        static runtime::ProgramCommandDescriptor last_command{-1, -1};

        runtime::CommandGenus current_genus = exec_obj_ptr->GetCommandGenus();
        const runtime::ProgramCommandDescriptor& current_command = exec_obj_ptr->GetCommandDesc();
        runtime::DebugCallbackReason debug_callback_reason = runtime::DebugCallbackReason::DEBUG_CALLBACK_UNKNOWN;

        if (current_genus == runtime::CommandGenus::CMD_GENUS_CALL_METHOD)
        { // Это пседокоманда-уведомление от функции ClassInstance::Call, вызывающей какой-либо метод
          // пользовательского (не встроенного) класса. Создаём запись о новом стековом кадре. Она пока будет
          // неполна, но позже будет дополнена при исполнении первой команды тела вызванного метода.            
            runtime::CallStackEntry new_stack_rec;
            new_stack_rec.call_command = last_command;
            new_stack_rec.info_data = *static_cast<runtime::PsevdoExecutable*>(exec_obj_ptr)->info_data_ptr;
            dbg_context->GetCallStack().push_back(new_stack_rec);
            is_wait_first_frame_command = true; // При следующем переходе к следующей строке будет захвачена
                                                // информация о первой исполняемой строке нового кадра.
            return;
        }

        if (current_genus == runtime::CommandGenus::CMD_GENUS_INITIALIZE)
        {
            dbg_context->GetCallStack().clear();
            dbg_context->GetCallStack().push_back({}); // Здесь создаём запись о корневом стековом кадре
            runtime::CallStackEntry& call_stack_entry = dbg_context->GetCallStack().back();
            call_stack_entry.call_command = current_command;
            call_stack_entry.first_command = current_command;
            call_stack_entry.closure_ptr = &closure;
            call_stack_entry.info_data = "root";
            is_wait_first_frame_command = false;
        }

        if (current_genus != runtime::CommandGenus::CMD_GENUS_CALL_METHOD &&
            current_command.module_string_number >= 0)
        {
            last_command = current_command;
        }

        if (context.GetLastCommandDesc() != current_command)
        { // Исполнение перешло к следующей строке исходного текста
            if (is_wait_first_frame_command)
            { // Сохраняем информацию о положении первой исполняемой строки очередного стекового кадра.
              // Сама запись о кадре была создана ранее при выполнении функции ClassInstance::Call, вызывающей
              // какой-либо метод класса. Эта функция посылает уведомление о своём исполнении в виде псевдокоманды
              // рода runtime::CommandGenus::CMD_GENUS_CALL_METHOD.
                dbg_context->GetCallStack().back().first_command = current_command;
                dbg_context->GetCallStack().back().closure_ptr = &closure;
                is_wait_first_frame_command = false;
            }
            // Сначала проверим наличие здесь (на этой новой строке) точек останова

            // Если точек останова нет, возможно, выполняется тот или иной вид трассировки (пошагового исполнения)
            switch (dbg_context->GetDebugMode())
            {
            case runtime::DebugExecutionMode::DEBUG_STEP_IN:
                // Исполнение до начала следующей строки исходника
                debug_callback_reason = runtime::DebugCallbackReason::DEBUG_CALLBACK_STEP_IN;
                break;
            case runtime::DebugExecutionMode::DEBUG_STEP_OUT:
                // Исполнение до начала следующей строки исходника, обходя все вызовы функций
                if (dbg_context->GetCallStack().size() <= dbg_context->GetDebugStackCounter())
                    debug_callback_reason = runtime::DebugCallbackReason::DEBUG_CALLBACK_STEP_OUT;
                break;
            case runtime::DebugExecutionMode::DEBUG_EXIT_METHOD:
                // Запуск вплоть до оператора выхода из текущей функци
                if (dbg_context->GetCallStack().size() <= dbg_context->GetDebugStackCounter())
                {
                    if (current_genus == runtime::CommandGenus::CMD_GENUS_RETURN_FROM_METHOD ||
                        current_genus == runtime::CommandGenus::CMD_GENUS_AFTER_LAST_METHOD_STMT)
                        debug_callback_reason = runtime::DebugCallbackReason::DEBUG_CALLBACK_EXIT_METHOD;
                }
                break;
            default:
                break;
            }
        }

        if (debug_callback_reason != runtime::DebugCallbackReason::DEBUG_CALLBACK_UNKNOWN)
        { // Случилось какое-то отладочное событие, делаем отладочный звонок
            if (dbg_context->GetDebugCallback())
                dbg_context->SetDebugMode(dbg_context->GetDebugCallback()
                    (debug_callback_reason, exec_obj_ptr, closure, context));
            else
                dbg_context->SetDebugMode(runtime::DebugExecutionMode::DEBUG_NO_DEBUG);
            dbg_context->SetDebugStackCounter(dbg_context->GetCallStack().size());
        }

        if (current_genus == runtime::CommandGenus::CMD_GENUS_RETURN_FROM_METHOD ||
            current_genus == runtime::CommandGenus::CMD_GENUS_AFTER_LAST_METHOD_STMT)
        { // Здесь удаляется запись о выбывающем стековом кадре при исполнении команды выхода из метода
            dbg_context->GetCallStack().pop_back();
            if (debug_callback_reason != runtime::DebugCallbackReason::DEBUG_CALLBACK_UNKNOWN)
                dbg_context->DecDebugStackCounter();
        }
    }

    context.SetLastCommandDesc(exec_obj_ptr->GetCommandDesc());
    if (context.IsTerminate())
        ThrowRuntimeError(exec_obj_ptr, ThrowMessageNumber::THRM_URGENT_TERMINATE);
}

namespace
{
    runtime::LinkageValue ConvertToLinkageValue(const runtime::ObjectHolder& input_object)
    {
        if (runtime::Bool* bool_ptr = input_object.TryAs<runtime::Bool>())
        {
            return bool_ptr->GetValue();
        }
        else if (runtime::Number* number_ptr = input_object.TryAs<runtime::Number>())
        {
            if (number_ptr->IsInt())
                return number_ptr->GetIntValue();
            else if (number_ptr->IsDouble())
                return number_ptr->GetDoubleValue();
        }
        else if (runtime::String* string_ptr = input_object.TryAs<runtime::String>())
        {
            return string_ptr->GetValue();
        }
        return {};
    }

    runtime::ObjectHolder ConvertToObject(const runtime::LinkageValue& link_value)
    {
        if (std::holds_alternative<std::monostate>(link_value))
            return runtime::ObjectHolder::None();
        else if (std::holds_alternative<bool>(link_value))
            return runtime::ObjectHolder::Own(runtime::Bool(std::get<bool>(link_value)));
        else if (std::holds_alternative<int>(link_value))
            return runtime::ObjectHolder::Own(runtime::Number(std::get<int>(link_value)));
        else if (std::holds_alternative<double>(link_value))
            return runtime::ObjectHolder::Own(runtime::Number(std::get<double>(link_value)));
        else if (std::holds_alternative<std::string>(link_value))
            return runtime::ObjectHolder::Own(runtime::String(std::get<std::string>(link_value)));
        else
            return runtime::ObjectHolder::None();
    }
}  // namespace

namespace ast
{
    ObjectHolder DereferencePointerObject(const ObjectHolder& pointer_obj)
    {
        runtime::PointerObject* pointer_ptr = pointer_obj.TryAs<runtime::PointerObject>();
        if (pointer_ptr)
            if (ObjectHolder* deref_pointer = pointer_ptr->GetPointer())
                return *deref_pointer;
            else
                return ObjectHolder::None();
        else
            return pointer_obj;
    }

    ObjectHolder Assignment::Execute(Closure& closure, Context& context)
    {
        PrepareExecute(this, closure, context);
        return closure[var_] = rv_->Execute(closure, context);
    }

    Assignment::Assignment(std::string var, std::unique_ptr<Statement> rv) : var_(move(var)), rv_(move(rv))
    {}

    IndirectAssignment::IndirectAssignment(std::unique_ptr<Statement> object, std::string method,
        std::vector<std::unique_ptr<Statement>> args, std::unique_ptr<Statement> rv) :
        object_(move(object)), method_(move(method)), args_(move(args)), rv_(move(rv))
    {}

    ObjectHolder IndirectAssignment::Execute(Closure& closure, Context& context)
    {
        PrepareExecute(this, closure, context);
        ObjectHolder real_object = object_->Execute(closure, context);
        vector<ObjectHolder> real_args;
        for (auto& cur_arg_ptr : args_) // Вычисляем истинные значения аргументов метода
            real_args.push_back(cur_arg_ptr->Execute(closure, context));

        ObjectHolder target_field = real_object.TryAs<runtime::CommonClassInstance>()->
                                    Call(method_, real_args, context);
        runtime::PointerObject* target_ptr = target_field.TryAs<runtime::PointerObject>();
        if (target_ptr)
        {
            if (ObjectHolder * deref_ptr = target_ptr->GetPointer())
                deref_ptr->ModifyData(DereferencePointerObject(rv_->Execute(closure, context)));
        }
        else
        {
            ThrowRuntimeError(this, ThrowMessageNumber::THRM_INDIRECT_ASSIGN_ERROR);
        }

        return target_field;
    }

    VariableValue::VariableValue(const std::string& var_name)
    {
        dotted_ids_.push_back(var_name);
    }

    VariableValue::VariableValue(std::vector<std::string> dotted_ids) : dotted_ids_(move(dotted_ids))
    {}

    ObjectHolder VariableValue::Execute(Closure& closure, [[maybe_unused]] Context& context)
    {
        PrepareExecute(this, closure, context);
        size_t i = 1;
        Closure* cur_closure_ptr = &closure;
        runtime::ClassInstance* cur_class_instance_ptr = nullptr;
        for (const string id_name : dotted_ids_)
        {
            if (!cur_closure_ptr->count(id_name))
                ThrowRuntimeError(this, ThrowMessageNumber::THRM_VARIABLE_NOT_FOUND);

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
                    return ConvertToObject(context.GetExternalLinkage()
                        (runtime::LinkCallReason::CALL_REASON_READ_FIELD, id_name, {}));
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
        PrepareExecute(this, closure, context);
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
        PrepareExecute(this, closure, context);
        ObjectHolder real_object = object_->Execute(closure, context);
        vector<ObjectHolder> real_args;
        for (auto& cur_arg_ptr : args_)
        { // Вычисляем истинные значения аргументов метода
            real_args.push_back(cur_arg_ptr->Execute(closure, context));
        }

        runtime::CommonClassInstance* common_class_ptr = real_object.TryAs<runtime::CommonClassInstance>();
        if (!common_class_ptr)
            ThrowRuntimeError(this, ThrowMessageNumber::THRM_METHOD_NOT_FOUND);

        ObjectHolder result = DereferencePointerObject(common_class_ptr->Call(method_, real_args, context));

        runtime::ClassInstance* real_class_ptr = dynamic_cast<runtime::ClassInstance*>(common_class_ptr);
        if (real_class_ptr)
        {  // Требуется вызвать метод объекта общего типа (определенного программно)
            if (real_class_ptr->GetClassName() == EXTERNAL_LINK_CLASS_NAME && context.GetExternalLinkage())
            {  // Вызов звонковой функции при вызове метода объекта "__external"
                vector<runtime::LinkageValue> real_args_lv;
                for (auto& cur_real_arg : real_args)
                    real_args_lv.push_back(ConvertToLinkageValue(cur_real_arg));

                return ConvertToObject(context.GetExternalLinkage()
                    (runtime::LinkCallReason::CALL_REASON_CALL_METHOD, method_, real_args_lv));
            }
            else
            {
                return result;
            }
        }
        else
        {
            return result;
        }
    }

    ObjectHolder Stringify::Execute(Closure& closure, Context& context)
    {
        PrepareExecute(this, closure, context);
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
        PrepareExecute(this, closure, context);
        runtime::ObjectHolder real_lhs(lhs_->Execute(closure, context));
        runtime::ObjectHolder real_rhs(rhs_->Execute(closure, context));

        if (real_lhs.TryAs<runtime::Number>() && real_rhs.TryAs<runtime::Number>())
        {
            runtime::Number result = *real_lhs.TryAs<runtime::Number>() + *real_rhs.TryAs<runtime::Number>();
            return ObjectHolder::Own<runtime::Number>(move(result));
        }
        else if (real_lhs.TryAs<runtime::String>() && real_rhs.TryAs<runtime::String>())
        {
            string result = real_lhs.TryAs<runtime::String>()->GetValue() + real_rhs.TryAs<runtime::String>()->GetValue();
            return ObjectHolder::Own<runtime::String>(result);
        }
        else if (real_lhs.TryAs<runtime::ClassInstance>())
        {
            runtime::CommonClassInstance *lhs_class_ptr = real_lhs.TryAs<runtime::ClassInstance>();
            if (lhs_class_ptr->HasMethod(ADD_METHOD, 1))
                return lhs_class_ptr->Call(ADD_METHOD, {real_rhs}, context);
            else
                ThrowRuntimeError(this, ThrowMessageNumber::THRM_IMPOSSIBLE_ADDITION);
        }
        else
        {
            ThrowRuntimeError(this, ThrowMessageNumber::THRM_IMPOSSIBLE_ADDITION);
        }
    }

    // Поддерживается вычитание:
    //  число - число
    // Если lhs и rhs - не числа, выбрасывается исключение runtime_error
    ObjectHolder Sub::Execute(Closure& closure, Context& context)
    {
        PrepareExecute(this, closure, context);
        runtime::ObjectHolder real_lhs(lhs_->Execute(closure, context));
        runtime::ObjectHolder real_rhs(rhs_->Execute(closure, context));

        if (real_lhs.TryAs<runtime::Number>() && real_rhs.TryAs<runtime::Number>())
        {
            runtime::Number result = (*real_lhs.TryAs<runtime::Number>()) - (*real_rhs.TryAs<runtime::Number>());
            return ObjectHolder::Own<runtime::Number>(move(result));
        }
        else
        {
            ThrowRuntimeError(this, ThrowMessageNumber::THRM_IMPOSSIBLE_SUBTRACTION);
        }
    }

    ObjectHolder Mult::Execute(Closure& closure, Context& context)
    {
        PrepareExecute(this, closure, context);
        runtime::ObjectHolder real_lhs(lhs_->Execute(closure, context));
        runtime::ObjectHolder real_rhs(rhs_->Execute(closure, context));

        if (real_lhs.TryAs<runtime::Number>() && real_rhs.TryAs<runtime::Number>())
        {
            runtime::Number result = (*real_lhs.TryAs<runtime::Number>()) * (*real_rhs.TryAs<runtime::Number>());
            return ObjectHolder::Own<runtime::Number>(move(result));
        }
        else
        {
            ThrowRuntimeError(this, ThrowMessageNumber::THRM_IMPOSSIBLE_MULTIPLICATION);
        }
    }

    ObjectHolder Div::Execute(Closure& closure, Context& context)
    {
        PrepareExecute(this, closure, context);
        runtime::ObjectHolder real_lhs(lhs_->Execute(closure, context));
        runtime::ObjectHolder real_rhs(rhs_->Execute(closure, context));
        runtime::Number* real_rhs_number_ptr = real_rhs.TryAs<runtime::Number>();

        if (real_lhs.TryAs<runtime::Number>() && real_rhs_number_ptr)
        {
            if (real_rhs_number_ptr->IsInt() && !real_rhs_number_ptr->GetIntValue())
                ThrowRuntimeError(this, ThrowMessageNumber::THRM_DIVISION_BY_ZERO);
            runtime::Number result = (*real_lhs.TryAs<runtime::Number>()) / (*real_rhs_number_ptr);
            return ObjectHolder::Own<runtime::Number>(move(result));
        }
        else
        {
            ThrowRuntimeError(this, ThrowMessageNumber::THRM_IMPOSSIBLE_DIVISION);
        }
    }

    ObjectHolder ModuloDiv::Execute(Closure& closure, Context& context)
    {
        PrepareExecute(this, closure, context);
        runtime::ObjectHolder real_lhs(lhs_->Execute(closure, context));
        runtime::ObjectHolder real_rhs(rhs_->Execute(closure, context));
        runtime::Number* real_rhs_number_ptr = real_rhs.TryAs<runtime::Number>();

        if (real_lhs.TryAs<runtime::Number>() && real_rhs_number_ptr)
        {
            if (real_rhs_number_ptr->IsInt() && !real_rhs_number_ptr->GetIntValue())
                ThrowRuntimeError(this, ThrowMessageNumber::THRM_DIVISION_BY_ZERO);
            runtime::Number result = (*real_lhs.TryAs<runtime::Number>()) % (*real_rhs_number_ptr);
            return ObjectHolder::Own<runtime::Number>(move(result));
        }
        else
        {
            ThrowRuntimeError(this, ThrowMessageNumber::THRM_IMPOSSIBLE_MODULO_DIVISION);
        }
    }

    ObjectHolder Compound::Execute(Closure& closure, Context& context)
    {
        PrepareExecute(this, closure, context);
        for (auto& cur_statement_ptr : comp_body_)
            cur_statement_ptr->Execute(closure, context);
        return {};
    }

    void Compound::AddStatement(std::unique_ptr<Statement> stmt)
    { // Добавляет очередную инструкцию в конец составной инструкции
        if (Compound* compound_stmt_ptr = dynamic_cast<Compound*>(stmt.get()))
            last_body_command_desc_ =  compound_stmt_ptr->GetLastCommandDesc();           
        else
            last_body_command_desc_ =  stmt->GetCommandDesc();
        comp_body_.push_back(std::move(stmt));
    }

    ObjectHolder Return::Execute(Closure& closure, Context& context)
    {
        PrepareExecute(this, closure, context);
        throw ReturnResult(statement_->Execute(closure, context));
    }

    ObjectHolder ReturnRef::ExecuteForVariable(Closure& closure, [[maybe_unused]] Context& context)
    {
        size_t i = 1;
        Closure* cur_closure_ptr = &closure;
        runtime::ClassInstance* cur_class_instance_ptr = nullptr;
        for (const string id_name : dotted_ids_)
        {
            if (!cur_closure_ptr->count(id_name))
                ThrowRuntimeError(this, ThrowMessageNumber::THRM_VARIABLE_NOT_FOUND);

            if (i++ < dotted_ids_.size())
            {
                cur_class_instance_ptr = cur_closure_ptr->at(id_name).TryAs<runtime::ClassInstance>();
                cur_closure_ptr = &(cur_class_instance_ptr->Fields());
            }
            else
            {
                throw ReturnResult(runtime::ObjectHolder::Own(
                    runtime::PointerObject(&cur_closure_ptr->at(id_name))));
            }
        }
        return {};
    }

    ObjectHolder ReturnRef::ExecuteForMethod(Closure& closure, Context& context)
    {
        ObjectHolder real_object = object_->Execute(closure, context);
        vector<ObjectHolder> real_args;
        for (auto& cur_arg_ptr : args_) // Вычисляем истинные значения аргументов метода
            real_args.push_back(cur_arg_ptr->Execute(closure, context));

        ObjectHolder target_field = real_object.TryAs<runtime::CommonClassInstance>()->
                                    Call(method_, real_args, context);
        if (target_field.TryAs<runtime::PointerObject>())
            throw ReturnResult(move(target_field));
        else
            ThrowRuntimeError(this, ThrowMessageNumber::THRM_INDIRECT_ASSIGN_ERROR);
    }

    ObjectHolder ReturnRef::Execute(Closure& closure, [[maybe_unused]] Context& context)
    {
        PrepareExecute(this, closure, context);
        if (dotted_ids_.size())
            return ExecuteForVariable(closure, context);
        else
            return ExecuteForMethod(closure, context);
    }

    ObjectHolder Break::Execute(Closure& closure, Context& context)
    {
        PrepareExecute(this, closure, context);
        throw TerminateLoop(TerminateLoopReason::TERMINATE_LOOP_BREAK);
    }

    ObjectHolder Continue::Execute(Closure& closure, Context& context)
    {
        PrepareExecute(this, closure, context);
        throw TerminateLoop(TerminateLoopReason::TERMINATE_LOOP_CONTINUE);
    }

    ClassDefinition::ClassDefinition(ObjectHolder cls) : cls_(move(cls))
    {}

    ObjectHolder ClassDefinition::Execute(Closure& closure, [[maybe_unused]] Context& context)
    {
        PrepareExecute(this, closure, context);
        closure[cls_.TryAs<runtime::Class>()->GetName()] = cls_;
        return cls_;
    }

    string ClassDefinition::GetClassName() const
    {
        return cls_.TryAs<runtime::Class>()->GetName();
    }

    std::vector<std::pair<std::string, size_t>> ClassDefinition::GetMethodsDesc() const
    {
        return cls_.TryAs<runtime::Class>()->GetMethodsDesc();
    }

    FieldAssignment::FieldAssignment(VariableValue object, std::string field_name,
                                     std::unique_ptr<Statement> rv) :
                                     object_(move(object)), field_name_(move(field_name)),
                                     rv_(move(rv))
    {}

    ObjectHolder FieldAssignment::Execute(Closure& closure, Context& context)
    { // Присваивает полю object.field_name значение выражения rv
        PrepareExecute(this, closure, context);
        runtime::ClassInstance* target_object_ptr = nullptr;
        ObjectHolder target_object_holder(object_.Execute(closure, context));
        if (target_object_holder)
        {
            target_object_ptr = target_object_holder.TryAs<runtime::ClassInstance>();
            ObjectHolder value_holder = rv_->Execute(closure, context);
            if (target_object_ptr->GetClassName() == EXTERNAL_LINK_CLASS_NAME && 
                context.GetExternalLinkage() && field_name_.size() && value_holder)
            { // Вызов звонковой функции при записи полей объекта "__external"
                context.GetExternalLinkage()(runtime::LinkCallReason::CALL_REASON_WRITE_FIELD,
                                             field_name_, {ConvertToLinkageValue(value_holder)});
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
        PrepareExecute(this, closure, context);
        if (runtime::IsTrue(condition_->Execute(closure, context)))
            return if_body_->Execute(closure, context);
        else
            if (else_body_)
                return else_body_->Execute(closure, context);
            else
                return {};
    }

    While::While(std::unique_ptr<Statement> condition, std::unique_ptr<Statement> while_body) :
        condition_(move(condition)), while_body_(move(while_body))
    {}

    ObjectHolder While::Execute(Closure& closure, Context& context)
    {
        PrepareExecute(this, closure, context);
        ObjectHolder result;
        while (runtime::IsTrue(condition_->Execute(closure, context)))
        {
            try
            {
                result = while_body_->Execute(closure, context);
            }
            catch (TerminateLoop& ter_loop)
            {
                if (ter_loop.terminate_loop_reason_ == TerminateLoopReason::TERMINATE_LOOP_BREAK)
                    break;            
                else if (ter_loop.terminate_loop_reason_ == TerminateLoopReason::TERMINATE_LOOP_CONTINUE)
                    continue;
                else
                    throw;
            }
        }

        return result;
    }

    ObjectHolder ShiftLeft::Execute(Closure& closure, Context& context)
    {
        PrepareExecute(this, closure, context);
        ObjectHolder real_lhs = lhs_->Execute(closure, context);
        ObjectHolder real_rhs = rhs_->Execute(closure, context);

        if (real_lhs.TryAs<runtime::Number>() && real_rhs.TryAs<runtime::Number>())
        {
            runtime::Number result = *real_lhs.TryAs<runtime::Number>() << *real_rhs.TryAs<runtime::Number>();
            return ObjectHolder::Own<runtime::Number>(move(result));
        }
        else if (real_lhs.TryAs<runtime::String>())
        {
            string str_result;
            if (real_rhs.TryAs<runtime::String>())
            {
                const string& rhs_str = real_rhs.TryAs<runtime::String>()->GetValue();
                size_t i = 0;
                for (unsigned char c : real_lhs.TryAs<runtime::String>()->GetValue())
                {
                    if (i < rhs_str.size())
                        str_result += c << rhs_str[i++];
                    else
                        str_result += c;
                }
            }
            else if (real_rhs.TryAs<runtime::Number>())
            {
                unsigned char shift_cnt = real_rhs.TryAs<runtime::Number>()->GetIntValue();
                for (unsigned char c : real_lhs.TryAs<runtime::String>()->GetValue())
                    str_result += c << shift_cnt;
            }
            else
            {
                ThrowRuntimeError(this, ThrowMessageNumber::THRM_SHIFT_INVALID_PARAMS);
            }
            return ObjectHolder::Own<runtime::String>(move(str_result));
        }
        else
        {
            ThrowRuntimeError(this, ThrowMessageNumber::THRM_SHIFT_INVALID_PARAMS);
        }
    }

    ObjectHolder ShiftRight::Execute(Closure& closure, Context& context)
    {
        PrepareExecute(this, closure, context);
        ObjectHolder real_lhs = lhs_->Execute(closure, context);
        ObjectHolder real_rhs = rhs_->Execute(closure, context);

        if (real_lhs.TryAs<runtime::Number>() && real_rhs.TryAs<runtime::Number>())
        {
            runtime::Number result = *real_lhs.TryAs<runtime::Number>() >> *real_rhs.TryAs<runtime::Number>();
            return ObjectHolder::Own<runtime::Number>(move(result));
        }
        else if (real_lhs.TryAs<runtime::String>())
        {
            string str_result;
            if (real_rhs.TryAs<runtime::String>())
            {
                const string& rhs_str = real_rhs.TryAs<runtime::String>()->GetValue();
                size_t i = 0;
                for (unsigned char c : real_lhs.TryAs<runtime::String>()->GetValue())
                {
                    if (i < rhs_str.size())
                        str_result += c >> rhs_str[i++];
                    else
                        str_result += c;
                }
            }
            else if (real_rhs.TryAs<runtime::Number>())
            {
                unsigned char shift_cnt = real_rhs.TryAs<runtime::Number>()->GetIntValue();
                for (unsigned char c : real_lhs.TryAs<runtime::String>()->GetValue())
                    str_result += c >> shift_cnt;
            }
            else
            {
                ThrowRuntimeError(this, ThrowMessageNumber::THRM_SHIFT_INVALID_PARAMS);
            }
            return ObjectHolder::Own<runtime::String>(move(str_result));
        }
        else
        {
            ThrowRuntimeError(this, ThrowMessageNumber::THRM_SHIFT_INVALID_PARAMS);
        }
    }

    ObjectHolder Or::Execute(Closure& closure, Context& context)
    {
        PrepareExecute(this, closure, context);
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
        PrepareExecute(this, closure, context);
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

    ObjectHolder Xor::Execute(Closure& closure, Context& context)
    {
        PrepareExecute(this, closure, context);
        bool bool_lhs = runtime::IsTrue(lhs_->Execute(closure, context));
        bool bool_rhs = runtime::IsTrue(rhs_->Execute(closure, context));
        return ObjectHolder::Own(runtime::Bool(bool_lhs != bool_rhs));
    }

    ObjectHolder BitwiseOr::Execute(Closure& closure, Context& context)
    {
        PrepareExecute(this, closure, context);
        runtime::ObjectHolder real_lhs(lhs_->Execute(closure, context));
        runtime::ObjectHolder real_rhs(rhs_->Execute(closure, context));

        if (real_lhs.TryAs<runtime::Number>() && real_rhs.TryAs<runtime::Number>())
        {
            runtime::Number result = *real_lhs.TryAs<runtime::Number>() | *real_rhs.TryAs<runtime::Number>();
            return ObjectHolder::Own<runtime::Number>(move(result));
        }
        else if (real_lhs.TryAs<runtime::String>() && real_rhs.TryAs<runtime::String>())
        {
            string real_str_lhs = real_lhs.TryAs<runtime::String>()->GetValue();
            string real_str_rhs = real_rhs.TryAs<runtime::String>()->GetValue();
            string result;
            for (size_t i = 0; i < max(real_str_lhs.size(), real_str_rhs.size()); ++i)
            {
                char lhs_char = 0, rhs_char = 0;
                if (i < real_str_lhs.size())
                    lhs_char = real_str_lhs[i];
                if (i < real_str_rhs.size())
                    rhs_char = real_str_rhs[i];
                result += lhs_char | rhs_char;
            }
            return ObjectHolder::Own<runtime::String>(result);
        }
        else
        {
            return ObjectHolder::Own(runtime::Bool(runtime::IsTrue(real_lhs) || runtime::IsTrue(real_rhs)));
        }
    }

    ObjectHolder BitwiseAnd::Execute(Closure& closure, Context& context)
    {
        PrepareExecute(this, closure, context);
        runtime::ObjectHolder real_lhs(lhs_->Execute(closure, context));
        runtime::ObjectHolder real_rhs(rhs_->Execute(closure, context));

        if (real_lhs.TryAs<runtime::Number>() && real_rhs.TryAs<runtime::Number>())
        {
            runtime::Number result = *real_lhs.TryAs<runtime::Number>() & *real_rhs.TryAs<runtime::Number>();
            return ObjectHolder::Own<runtime::Number>(move(result));
        }
        else if (real_lhs.TryAs<runtime::String>() && real_rhs.TryAs<runtime::String>())
        {
            string real_str_lhs = real_lhs.TryAs<runtime::String>()->GetValue();
            string real_str_rhs = real_rhs.TryAs<runtime::String>()->GetValue();
            string result;
            for (size_t i = 0; i < max(real_str_lhs.size(), real_str_rhs.size()); ++i)
            {
                char lhs_char = 0, rhs_char = 0;
                if (i < real_str_lhs.size())
                    lhs_char = real_str_lhs[i];
                if (i < real_str_rhs.size())
                    rhs_char = real_str_rhs[i];
                result += lhs_char & rhs_char;
            }
            return ObjectHolder::Own<runtime::String>(result);
        }
        else
        {
            return ObjectHolder::Own(runtime::Bool(runtime::IsTrue(real_lhs) && runtime::IsTrue(real_rhs)));
        }
    }

    ObjectHolder BitwiseXor::Execute(Closure& closure, Context& context)
    {
        PrepareExecute(this, closure, context);
        runtime::ObjectHolder real_lhs(lhs_->Execute(closure, context));
        runtime::ObjectHolder real_rhs(rhs_->Execute(closure, context));

        if (real_lhs.TryAs<runtime::Number>() && real_rhs.TryAs<runtime::Number>())
        {
            runtime::Number result = *real_lhs.TryAs<runtime::Number>() ^ *real_rhs.TryAs<runtime::Number>();
            return ObjectHolder::Own<runtime::Number>(move(result));
        }
        else if (real_lhs.TryAs<runtime::String>() && real_rhs.TryAs<runtime::String>())
        {
            string real_str_lhs = real_lhs.TryAs<runtime::String>()->GetValue();
            string real_str_rhs = real_rhs.TryAs<runtime::String>()->GetValue();
            string result;
            for (size_t i = 0; i < max(real_str_lhs.size(), real_str_rhs.size()); ++i)
            {
                char lhs_char = 0, rhs_char = 0;
                if (i < real_str_lhs.size())
                    lhs_char = real_str_lhs[i];
                if (i < real_str_rhs.size())
                    rhs_char = real_str_rhs[i];
                result += lhs_char ^ rhs_char;
            }
            return ObjectHolder::Own<runtime::String>(result);
        }
        else
        {
            return ObjectHolder::Own(runtime::Bool(runtime::IsTrue(real_lhs) != runtime::IsTrue(real_rhs)));
        }
    }

    ObjectHolder Complement::Execute(Closure& closure, Context& context)
    {
        PrepareExecute(this, closure, context);
        ObjectHolder real_arg = argument_->Execute(closure, context);

        if (runtime::Number* real_arg_number_ptr = real_arg.TryAs<runtime::Number>())
            return ObjectHolder::Own<runtime::Number>(~(*real_arg_number_ptr));
        
        if (runtime::String* real_arg_string_ptr = real_arg.TryAs<runtime::String>())
        {
            std::string result_str;
            for (char c : real_arg_string_ptr->GetValue())
                result_str += ~c;
            return ObjectHolder::Own(runtime::String(std::move(result_str)));
        }

        return ObjectHolder::Own(runtime::Bool(!runtime::IsTrue(real_arg)));
    }

    ObjectHolder Not::Execute(Closure& closure, Context& context)
    {
        PrepareExecute(this, closure, context);
        ObjectHolder real_arg = argument_->Execute(closure, context);
        return ObjectHolder::Own(runtime::Bool(!runtime::IsTrue(real_arg)));
    }

    Comparison::Comparison(Comparator cmp, unique_ptr<Statement> lhs, unique_ptr<Statement> rhs)
        : BinaryOperation(std::move(lhs), std::move(rhs)), cmp_(cmp)
    {}

    ObjectHolder Comparison::Execute(Closure& closure, Context& context)
    {
        PrepareExecute(this, closure, context);
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
        PrepareExecute(this, closure, context);
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
    {
        dummy_statement_->SetCommandGenus(runtime::CommandGenus::CMD_GENUS_AFTER_LAST_METHOD_STMT);
        runtime::ProgramCommandDescriptor after_body_command_desc;
        if (Compound* compound_body_ptr = dynamic_cast<Compound*>(body_.get()))
            after_body_command_desc = compound_body_ptr->GetLastCommandDesc();
        else
            after_body_command_desc = body_->GetCommandDesc();
        ++after_body_command_desc.module_string_number;
        dummy_statement_->SetCommandDesc(after_body_command_desc);
    }

    ObjectHolder MethodBody::Execute(Closure& closure, Context& context)
    {
        PrepareExecute(this, closure, context);
        try
        {
            body_->Execute(closure, context);
        }
        catch (ReturnResult ret_result)
        {
            return ret_result.ret_result_;
        }
        PrepareExecute(dummy_statement_.get(), closure, context);
        return runtime::ObjectHolder::None();
    }
}  // namespace ast

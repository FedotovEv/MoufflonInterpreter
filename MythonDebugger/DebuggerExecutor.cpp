
#include "DebuggerExecutor.h"

using namespace std;

void DebugController::operator()(bool is_run_debug)
{ // Основное тело контроллера, внутри которого будет выполняться весь рабочий процесс и
  // находиться управление на всём протяжении хода отладки.
  // Типовая предусмотренная логика работы контроллера подразумевает исполнение этого
  // оператора в отдельном потоке.
    using namespace placeholders;
    ControllerStatus temp_status;
    temp_status.run_status = ControllerRunStatus::CONTROL_STATUS_UNKNOWN;
    controller_status_ = temp_status;
    controller_result_ = ControllerResult{};
    controller_comand_ = ControllerCommand::CONTROL_UNKNOWN;
    debug_context_.Clear();
    SendDebugMessage();

    parse::TrivialParseContext parse_context(true);
    runtime::Closure closure;

    try
    {
        parse::Lexer input_lexer(debugger_project_.GetLexerInputStream());
        auto program = ParseProgram(input_lexer, parse_context);    
        // Настраиваем debug_context под требуемый режим работы - отладочная звонковая функция
        // и начальный режим работы. Для запуска под отладкой запуск производится с начальным режимом
        // DEBUG_STEP_IN (должна произойти остановка на самой первой строке программы), а для исполнения
        // без контроля отладчика используется режим DEBUG_NO_DEBUG.
        auto bind_debug_callback = bind(&DebugController::DebugCallbackImpl, this, _1, _2, _3, _4);
        debug_context_.SetDebugCallback(bind_debug_callback);
        if (is_run_debug)
            debug_context_.SetDebugMode(runtime::DebugExecutionMode::DEBUG_STEP_IN);
        else
            debug_context_.SetDebugMode(runtime::DebugExecutionMode::DEBUG_NO_DEBUG);

        temp_status.run_status = ControllerRunStatus::CONTROL_STATUS_RUNNING;
        controller_status_ = temp_status;
        SendDebugMessage();
        program->Execute(closure, debug_context_);
        controller_result_ = ExecutionRetcode::RETCODE_EXIT;
    }
    catch (ParseError& parse_err)
    {
        controller_result_ = move(parse_err);
    }
    catch (runtime_error& runtime_err)
    {
        controller_result_ = move(runtime_err);
    }
    temp_status.run_status = ControllerRunStatus::CONTROL_STATUS_FINISHED;
    controller_status_ = temp_status;
    SendDebugMessage();
}

void DebugController::CommitCommand(ControllerCommand do_command)
{
    controller_comand_ = do_command;
    if (do_command == ControllerCommand::CONTROL_TERMINATE_PROGRAM)
    {
        ControllerStatus cur_status = controller_status_;
        if (cur_status.run_status != ControllerRunStatus::CONTROL_STATUS_STOPPED &&
            cur_status.run_status != ControllerRunStatus::CONTROL_STATUS_FINISHED)
            debug_context_.SetTerminate();
    }
    cond_var_.notify_one();
}

DebugController::ControllerResult DebugController::GetResult()
{
    ControllerStatus cur_status = controller_status_;
    if (cur_status.run_status == ControllerRunStatus::CONTROL_STATUS_STOPPED ||
        cur_status.run_status == ControllerRunStatus::CONTROL_STATUS_FINISHED)
        return controller_result_;
    else
        return {};
}

runtime::DebugExecutionMode DebugController::DebugCallbackImpl(runtime::DebugCallbackReason call_reason,
                runtime::Executable* exec_obj_ptr, runtime::Closure& closure, runtime::Context& context)
{
    ControllerStatus temp_status;
    temp_status.callback_reason = call_reason;
    temp_status.run_status = ControllerRunStatus::CONTROL_STATUS_STOPPED;
    temp_status.stop_point = exec_obj_ptr->GetCommandDesc();
    controller_status_ = temp_status;
    SendDebugMessage();
    ControllerCommand cur_command;
    unique_lock lk(cond_mutex_);
    cond_var_.wait(lk, [this, &cur_command]
        {
            cur_command = controller_comand_;
            return cur_command != ControllerCommand::CONTROL_UNKNOWN;
        });

    controller_comand_ = ControllerCommand::CONTROL_UNKNOWN;
    runtime::DebugExecutionMode result_run_mode;
    switch (cur_command)
    {
    case ControllerCommand::CONTROL_STEP_IN:
        result_run_mode = runtime::DebugExecutionMode::DEBUG_STEP_IN;
        break;
    case ControllerCommand::CONTROL_STEP_OUT:
        result_run_mode = runtime::DebugExecutionMode::DEBUG_STEP_OUT;
        break;
    case ControllerCommand::CONTROL_EXIT_METHOD:
        result_run_mode = runtime::DebugExecutionMode::DEBUG_EXIT_METHOD;
        break;
    case ControllerCommand::CONTROL_TERMINATE_PROGRAM:
        debug_context_.SetTerminate();
        [[fallthrough]];
    case ControllerCommand::CONTROL_EXECUTE_PROGRAM:
        [[fallthrough]];
    default:
        result_run_mode = runtime::DebugExecutionMode::DEBUG_SIMPLE_RUN;
        break;
    }

    temp_status.run_status = ControllerRunStatus::CONTROL_STATUS_RUNNING;
    controller_status_ = temp_status;
    SendDebugMessage();
    return result_run_mode;
}

void DebugController::SendDebugMessage()
{
    ControllerStatus temp_status = controller_status_;
    wxCommandEvent* event_ptr = new wxCommandEvent(DEBUG_EVENT_TYPE, wxID_MAIN_WINDOW);
    event_ptr->SetEventObject(debug_output_.GetDebugOutputWindow());
    event_ptr->SetInt(static_cast<int>(temp_status.run_status));
    event_ptr->SetExtraLong(static_cast<long>(temp_status.callback_reason));
    wxQueueEvent(parent_window_, event_ptr);
}

#pragma once

#include <iostream>
#include <streambuf>
#include <future>
#include <mutex>
#include <condition_variable>

#include "gui/OutputDebuggerWindow.h"
#include "DebuggerDataStruct.h"

wxDECLARE_EVENT(DEBUG_EVENT_TYPE, wxCommandEvent);

class DebugOutput
{
public:
    DebugOutput(wxWindow* parent_window) :
        parent_window_(parent_window),
        debug_output_window_(new OutputDebuggerWindowImpl(parent_window_)),
        output_text_ctrl_(debug_output_window_->GetOutputTextCtrl()),
        debug_stream_(output_text_ctrl_)
    {}
    
    ~DebugOutput()
    {
        debug_output_window_->Destroy();
        output_text_ctrl_ = nullptr;
        debug_output_window_ = nullptr;
    }

    void Show()
    {
        debug_output_window_->Show();
    }
    
    void Hide()
    {
        debug_output_window_->Hide();
    }

    wxTextCtrl* GetOutputTextCtrl()
    {
        return output_text_ctrl_;
    }

    std::ostream& GetOutputStream()
    {
        return debug_stream_;
    }    

    wxWindow* GetDebugOutputWindow()
    {
        return debug_output_window_;
    }

private:
    wxWindow* parent_window_;
    OutputDebuggerWindowImpl* debug_output_window_; // Окно отладочной консоли
    wxTextCtrl* output_text_ctrl_; // Элемент управления этого окна, в который непосредственно выводится
                                   // печать отлаживаемой программы.
    std::ostream debug_stream_; // Поток, созданные на основе вышеупомянутого элемента управления. Всё,
                                // что направляется туда, попадает в окно отладочной консоли.
};

class DebugController
{
public:
    enum class ControllerCommand
    {
        CONTROL_UNKNOWN = 0,
        CONTROL_EXECUTE_PROGRAM = 1,
        CONTROL_TERMINATE_PROGRAM, // Немедленное аварийное завершение программы
        CONTROL_STEP_IN,
        CONTROL_STEP_OUT,
        CONTROL_EXIT_METHOD
    };

    enum class ControllerRunStatus
    {
        CONTROL_STATUS_UNKNOWN = 0,
        CONTROL_STATUS_RUNNING,
        CONTROL_STATUS_STOPPED,
        CONTROL_STATUS_FINISHED
    };

    struct ControllerStatus
    {
        ControllerRunStatus run_status = ControllerRunStatus::CONTROL_STATUS_UNKNOWN;
        runtime::DebugCallbackReason callback_reason;
        runtime::ProgramCommandDescriptor stop_point;
    };

    enum class ExecutionRetcode
    {
        RETCODE_EXIT = 0,
        RETCODE_TERMINATE = 1
    };

    using ControllerResult = std::variant<std::monostate, ExecutionRetcode, ParseError, std::runtime_error>;

    DebugController(wxWindow* parent_window, DebuggerProject& debugger_project) :
        parent_window_(parent_window),
        debug_output_(parent_window_),
        debugger_project_(debugger_project),
        debug_context_(debug_output_.GetOutputStream())
    {}
    
    ~DebugController() = default;
    
    DebuggerProject& GetDebuggerProject()
    {
        return debugger_project_;
    }

    LexerInputExImpl& GetInputLexer()
    {
        return debugger_project_.GetLexerInputStream();
    }

    runtime::DebugContext& GetContext()
    {
        return debug_context_;
    }

    DebugOutput& GetDebugOutput()
    {
        return debug_output_;
    }

    // operator() - основное тело контроллера, внутри которого будет выполняться весь рабочий процесс и
    // находиться управление на всём протяжении хода отладки.
    // Типовая предусмотренная логика работы контроллера подразумевает исполнение этого оператора в отдельном потоке.
    void operator()(bool is_run_debug);
    void CommitCommand(ControllerCommand do_command);

    ControllerCommand GetCommand()
    {
        return controller_comand_;
    }

    ControllerStatus GetStatus()
    {
        return controller_status_;
    }

    ControllerResult GetResult();

private:

    runtime::DebugExecutionMode DebugCallbackImpl(runtime::DebugCallbackReason call_reason,
        runtime::Executable* exec_obj_ptr, runtime::Closure& closure, runtime::Context& context);
    void SendDebugMessage();

    wxWindow* parent_window_;
    DebugOutput debug_output_; // Вспомогательное окно с консолью отлаживаемой программы
    DebuggerProject& debugger_project_; // Ссылка на отладочный проект с информацией об исходном тексте, точках
                                        // останова, надзоре и всем прочем, связанном с отлаживаемой программой.
    runtime::DebugContext debug_context_; // Исполнительский контекст, перенаправляющий вывод отлаживаемой внутрь
                                          // вспомогательного окна debug_output_.
    // Поле передачи очередной команды параллельному operator().
    std::atomic<ControllerCommand> controller_comand_{ControllerCommand::CONTROL_UNKNOWN};
    // Поле текущего состояния контроллера.
    std::atomic<ControllerStatus> controller_status_;
    ControllerResult controller_result_; // Поле для возврата кода завершения отладки.
    std::mutex cond_mutex_;
    std::condition_variable cond_var_;
};

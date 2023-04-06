#pragma once

#include "declares.h"
#include "runtime.h"

namespace runtime
{
    enum class DebugCallbackReason
    {
        DEBUG_CALLBACK_UNKNOWN = 0,
        DEBUG_CALLBACK_STEP_IN,
        DEBUG_CALLBACK_STEP_OUT,
        DEBUG_CALLBACK_EXIT_METHOD,
        DEBUG_CALLBACK_BREAPOINT
    };

    enum class DebugExecutionMode
    {
        DEBUG_NO_DEBUG = 0, // Режим без отладки
        DEBUG_SIMPLE_RUN, // Запуск под отладчиком до какой-либо контрольной точки
        DEBUG_STEP_IN, // Исполнение до начала следующей строки исходника
        DEBUG_STEP_OUT, // Исполнение до начала следующей строки исходника, обходя все вызовы функций
        DEBUG_EXIT_METHOD // Запуск вплоть до оператора выхода из текущей функции
    };

    using DebugCallback =
        std::function<DebugExecutionMode(DebugCallbackReason, Executable*, Closure&, Context&)>;

    class MYTHLON_INTERPRETER_PUBLIC DebugContext : public SimpleContext
    {
    public:
        explicit DebugContext(std::ostream& output, LinkageFunction external_link = LinkageFunction())
            : SimpleContext(output, std::move(external_link))
        {}

        DebugCallback& GetDebugCallback()
        {
            return debug_callback_;
        }

        void SetDebugCallback(DebugCallback debug_callback)
        {
            debug_callback_ = std::move(debug_callback);
        }

        DebugExecutionMode GetDebugMode()
        {
            return debug_exec_;
        }

        void SetDebugMode(DebugExecutionMode new_debug_exec)
        {
            debug_exec_ = new_debug_exec;
        }

        std::vector<CallStackEntry>& GetCallStack()
        {
            return call_stack_desc_;
        }

        size_t GetDebugStackCounter()
        {
            return debug_exec_stack_counter_;
        }

        void SetDebugStackCounter(size_t debug_exec_stack_counter)
        {
            debug_exec_stack_counter_ = debug_exec_stack_counter;
        }

        void DecDebugStackCounter()
        {
            if (debug_exec_stack_counter_)
                --debug_exec_stack_counter_;
        }

        void Clear()
        {
            SimpleContext::Clear();
            call_stack_desc_.clear();
            breakpoints_.clear();
            debug_exec_stack_counter_ = 0;
            debug_exec_ = DebugExecutionMode::DEBUG_NO_DEBUG;
        }

    private:
        DebugCallback debug_callback_;
        std::atomic<DebugExecutionMode> debug_exec_{DebugExecutionMode::DEBUG_NO_DEBUG};
        std::vector<ProgramCommandDescriptor> breakpoints_;
        // Описание текущего стека вызовов программы.
        std::vector<CallStackEntry> call_stack_desc_;
        // debug_exec_stack_counter_ - глубина стека, при которой был запущен
        // очередной шаг отладки (произведён запуск программы после отладочного звонка).
        size_t debug_exec_stack_counter_ = 0;
    };
}

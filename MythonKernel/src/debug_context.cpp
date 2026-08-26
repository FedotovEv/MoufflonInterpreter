
#include "debug_context.h"
#include "statement.h"
#include <iomanip>
#include <codecvt>
#include <locale>
#include <cassert>

namespace runtime
{
    std::ostream& operator<<(std::ostream& ostr, const ProgramCommandDescriptor& command_desc)
    {
        static constexpr int MODULE_ID_WIDTH = 4, MODULE_STRING_NUMBER_WIDTH = 5;

        ostr << "Module : " << std::setw(MODULE_ID_WIDTH) << command_desc.module_id
             << " : String : " << std::setw(MODULE_STRING_NUMBER_WIDTH) << command_desc.module_string_number;
        return ostr;
    }

    std::string CallbackReasonToString(DebugCallbackReason debug_callback_reason)
    {
        switch (debug_callback_reason)
        {
        case DebugCallbackReason::DEBUG_CALLBACK_INIT:
            return "Инициализация";
        case DebugCallbackReason::DEBUG_CALLBACK_STEP:
            return "Обычный шаг";
        case DebugCallbackReason::DEBUG_CALLBACK_EXIT_METHOD:
            return "Выход из метода";
        case DebugCallbackReason::DEBUG_CALLBACK_BREAKPOINT:
            return "Точка останова";
        case DebugCallbackReason::DEBUG_CALLBACK_CHECK_CONDITION:
            return "Проверка условия";
        default:
            return "Прочее";
        };
    }

    std::ostream& operator<<(std::ostream& ostr, DebugCallbackReason debug_callback_reason)
    {
        static constexpr int CALLBACK_REASON_WIDTH = 18;

        std::string callback_str = CallbackReasonToString(debug_callback_reason);
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        std::wstring wide_callback_str = converter.from_bytes(callback_str);

        int symb_delta = CALLBACK_REASON_WIDTH - static_cast<int>(wide_callback_str.size());
        if (symb_delta > 0)
            wide_callback_str += std::wstring(symb_delta, ' ');
        else if (symb_delta < 0)
            wide_callback_str = wide_callback_str.substr(0, CALLBACK_REASON_WIDTH);

        ostr << converter.to_bytes(wide_callback_str);
        return ostr;
    }
    
    std::string ExecutionModeToString(DebugExecutionMode debug_execution_mode)
    {
        switch (debug_execution_mode)
        {
        case DebugExecutionMode::DEBUG_NO_DEBUG:
            return "Без отладки";
        case DebugExecutionMode::DEBUG_SIMPLE_RUN:
            // Запуск под отладчиком до какой-либо контрольной точки.
            return "Простой запуск";
        case DebugExecutionMode::DEBUG_STEP_IN:
            // Исполнение до начала следующей строки исходника.
            return "Шаг с заходом";
        case DebugExecutionMode::DEBUG_STEP_OUT:
            return "Шаг с обходом";
        case DebugExecutionMode::DEBUG_EXIT_METHOD:
            return "До выхода из метода";
        default:
            return "Неизвестный";
        };
    }

    std::ostream& operator<<(std::ostream& ostr, DebugExecutionMode debug_execution_mode)
    {
        static constexpr int EXECUTION_MODE_WIDTH = 20;

        std::string exec_mode_str = ExecutionModeToString(debug_execution_mode);
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        std::wstring wide_exec_mode_str = converter.from_bytes(exec_mode_str);

        int symb_delta = EXECUTION_MODE_WIDTH - static_cast<int>(wide_exec_mode_str.size());
        if (symb_delta > 0)
            wide_exec_mode_str += std::wstring(symb_delta, ' ');
        else if (symb_delta < 0)
            wide_exec_mode_str = wide_exec_mode_str.substr(0, EXECUTION_MODE_WIDTH);

        ostr << converter.to_bytes(wide_exec_mode_str);
        return ostr;
    }

    #ifndef MYTHON_UNITHREAD
        // Многопоточный вариант отладочного контекста, предназначенный для одновременного доступа из нескольких параллельных
        // потоков (исполняющего и отлаживающего).
        // Макрос проверки допустимости индекса для некоторой существующей точки останова.
        #define CHECK_RET_BREAKPOINT(what_ret) \
            std::lock_guard lg(breakpoints_mutex_); \
            if (breakpoint_index >= breakpoints_.size() || !breakpoints_[breakpoint_index].position.IsValid()) \
                return (what_ret)

        // Макрос подстановки индекса истинной вершины стека вызовов вместо значения по умолчанию (если индекс не указан явно) и
        // проверки получившейся величины на допустимость.
        #define CHECK_RET_CALL_ENTRY(what_ret) \
            std::lock_guard lg(call_stack_desc_mutex_); \
            if (call_stack_desc_.empty()) \
                return (what_ret); \
            if (stack_entry_index == RESERVED_VALUE) \
                stack_entry_index = call_stack_desc_.size() - 1; \
            if (stack_entry_index >= call_stack_desc_.size()) \
                return (what_ret)
    #else
        // Однопоточный вариант отладочного контекста.
        // Макрос проверки допустимости индекса для некоторой существующей точки останова.
        #define CHECK_RET_BREAKPOINT(what_ret) \
            if (breakpoint_index >= breakpoints_.size() || breakpoints_[breakpoint_index].position == BreakpointDesc::DUMB_PROG_POS) \
                return (what_ret)

        // Макрос подстановки индекса истинной вершины стека вызовов вместо значения по умолчанию (если индекс не указан явно) и
        // проверки получившейся величины на допустимость.
        #define CHECK_RET_CALL_ENTRY(what_ret) \
            if (call_stack_desc_.empty()) \
                return (what_ret); \
            if (stack_entry_index == RESERVED_VALUE) \
                stack_entry_index = call_stack_desc_.size() - 1; \
            if (stack_entry_index >= call_stack_desc_.size()) \
                return (what_ret)
    #endif

    // Получение функтора-перехватчика отладочного звонка, назначенного при конструировании контекста.
    const DebugCallback& DebugContext::GetDebugCallback() const
    {
        return debug_callback_;
    }

    // Переустановка обработчика отладочных звонков.
    void DebugContext::SetDebugCallback(DebugCallback debug_callback)
    {
        debug_callback_ = std::move(debug_callback);
    }

    // Получение константной ссылки на весь массив, содержащий стек вызовов программы, исполняемой в отладочном режиме.
    const std::vector<CallStackEntry>& DebugContext::GetCallStack() const
    {
        #ifndef MYTHON_UNITHREAD
            call_stack_desc_ext_lock_.lock();
        #endif

        return call_stack_desc_;
    }

    void DebugContext::FreeCallStack() const
    {
        #ifndef MYTHON_UNITHREAD
            call_stack_desc_ext_lock_.unlock();
        #endif
    }

    // Захват и получение константной ссылки на полный список точек останова (включая и вакантные места, где таких бряков уже нет).
    // Для возобновления работы с этим списком всех прочих клиентов контекста, а также его внутренних механизмов, требуется как можно
    // быстрее освободить захваченный список вызовом FreeBreakpointsList().
    const std::vector<BreakpointDesc>& DebugContext::GetBreakpointsList() const
    {
        #ifndef MYTHON_UNITHREAD
            breakpoints_ext_lock_.lock();
        #endif

        return breakpoints_;
    }

    void DebugContext::FreeBreakpointsList() const
    {
        #ifndef MYTHON_UNITHREAD
            breakpoints_ext_lock_.unlock();
        #endif
    }

    // Функции-члены получения/установки режима возобновления (исполнения) программы.
    // Извлекатель текущего режима исполнения программы.
    DebugExecutionMode DebugContext::GetDebugMode() const
    {
        return debug_exec_;
    }

    // Установщик режима исполнения программы под контролем отладчика.
    void DebugContext::SetDebugMode(DebugExecutionMode new_debug_exec)
    {
        debug_exec_ = new_debug_exec;
    }

    // Функции-члены для работы с моделью стека вызовов свободных функций/методов отлаживаемой программы.
    // Количество записей в стеке вызовов на данный момент времени.
    size_t DebugContext::GetCallStackSize() const
    {
        #ifndef MYTHON_UNITHREAD
            std::lock_guard lg(call_stack_desc_mutex_);
        #endif

        return call_stack_desc_.size();
    }

    // Очистка стека вызовов.
    void DebugContext::ClearCallStack()
    {
        #ifndef MYTHON_UNITHREAD
            std::lock_guard lg(call_stack_desc_mutex_);
        #endif

        call_stack_desc_.clear();
        TestActiveStackIndex();
    }

    // Формирование/правка/удаление описателя отладочного стекового кадра метода или функции отлаживаемой программы.
    // Добавление нового стекового кадра на верхушку стека.
    size_t DebugContext::PushCallStackEntry(const CallStackEntry& new_stack_entry)
    {
        #ifndef MYTHON_UNITHREAD
            std::lock_guard lg(call_stack_desc_mutex_);
        #endif

        call_stack_desc_.push_back(new_stack_entry);
        return call_stack_desc_.size() - 1;
    }

    // Обновление стекового кадра с индексом stack_entry_index в соответствии с данными аргумента new_stack_entry.
    // При неуказании номера записи(вызове со значением stack_entry_index по умолчанию) будет обновлена запись в верхушке стека.
    size_t DebugContext::UpdateCallStackEntry(const CallStackEntry& new_stack_entry, size_t stack_entry_index)
    {
        CHECK_RET_CALL_ENTRY(RESERVED_VALUE);
        call_stack_desc_[stack_entry_index] = new_stack_entry;
        return stack_entry_index;
    }
    
    // Извлечение из исполнительского стека записи с индексом stack_entry_index. При неуказании номера записи (вызове со
    // значением stack_entry_index по умолчанию) будет извлечена запись из верхушки стека.
    std::pair<CallStackEntry, size_t> DebugContext::GetCallStackEntry(size_t stack_entry_index) const
    {
        std::pair<CallStackEntry, size_t> inv_ret_val = {{}, RESERVED_VALUE};
        CHECK_RET_CALL_ENTRY(inv_ret_val);
        return {call_stack_desc_[stack_entry_index], stack_entry_index};
    }
    
    // Выталкивание из стека его верхнего элемента, который также будт возвращён как результат работы данной функции.
    CallStackEntry DebugContext::PopCallStackEntry()
    {
        #ifndef MYTHON_UNITHREAD
            std::lock_guard lg(call_stack_desc_mutex_);
        #endif

        if (call_stack_desc_.empty())
        {
            active_stack_index_ = 0;
            return {};
        }

        CallStackEntry ret_value = call_stack_desc_.back();
        call_stack_desc_.pop_back();
        TestActiveStackIndex();

        return ret_value;
    }

    bool DebugContext::IsCallStackEntryValid(size_t stack_entry_index) const
    { // Проверка валидности некоторой записи в стеке вызовов исполняемой программы.
        CHECK_RET_CALL_ENTRY(true);
        return call_stack_desc_[stack_entry_index].is_valid;
    }

    // Проверка наличия ранее совершенного звонка типа DEBUG_CALLBACK_EXIT_METHOD при исполнении инструкций вызода из данной процедуры.
    bool DebugContext::IsCallbackEntryExitMethod(size_t stack_entry_index) const
    {
        CHECK_RET_CALL_ENTRY(true);
        return call_stack_desc_[stack_entry_index].is_method_exit_callback;
    }
    
    // Установка признака совершения звонка типа DEBUG_CALLBACK_EXIT_METHOD для процедуры, которой соответствует указанный кадр вызова.
    size_t DebugContext::SetCallbackEntryExitMethod(bool new_callback_status, size_t stack_entry_index)
    {
        CHECK_RET_CALL_ENTRY(RESERVED_VALUE);
        call_stack_desc_[stack_entry_index].is_method_exit_callback = new_callback_status;
        return stack_entry_index;
    }

    // Методы обслуживания указателя (индекса) положения в стеке вызовов текущей отлаживаемой функции
    // (то есть той, с которой в данный момент работает отладчик).
    // Возврат положения (индекса) текущего "активного" (принадлежащего отлаживаемой функции) стекового кадра.
    size_t DebugContext::GetActiveStackIndex() const
    {
        #ifndef MYTHON_UNITHREAD
            std::lock_guard lg(call_stack_desc_mutex_);
        #endif

        return active_stack_index_;
    }

    // Установка нового значения индекса "активного" стекового кадра.
    void DebugContext::SetActiveStackIndex(size_t new_active_stack_index)
    {
        #ifndef MYTHON_UNITHREAD
            std::lock_guard lg(call_stack_desc_mutex_);
        #endif

        active_stack_index_ = new_active_stack_index;
        TestActiveStackIndex();
    }

    // Проверяем "активность" последнего стекового кадра в их общем списке. Если "активен" именно последний такой
    // кадр, то, соответственно, "активной" (отлаживаемой) явлется как раз исполняемый в данный момент метод либо
    // свободная функция.
    bool DebugContext::IsLastFrameActive() const
    {
        #ifndef MYTHON_UNITHREAD
            std::lock_guard lg(call_stack_desc_mutex_);
        #endif

        return active_stack_index_ >= (call_stack_desc_.size() - 1);
    }

    // Возврат ранее сохранённого описания последнего встретившегося в потоке исполнения программы оператора вызова
    // подпрограммы.
    CallStatementDesc DebugContext::GetCallStatementDesc() const
    {
        #ifndef MYTHON_UNITHREAD
            std::lock_guard lg(last_call_statement_mutex_);
        #endif

        return last_call_statement_;
    }
    
    // Сохранение данных последнего встретившегося при исполнении программы оператора вызова подпрограммы.
    void DebugContext::SetCallStatementDesc(const CallStatementDesc& call_statement)
    {
        #ifndef MYTHON_UNITHREAD
            std::lock_guard lg(last_call_statement_mutex_);
        #endif

        last_call_statement_ = call_statement;
    }

    // Проверка индекса активного элемента стека вызовов на допустимость и, при необходимости, его коррекция.
    void DebugContext::TestActiveStackIndex()
    {
        if (call_stack_desc_.empty())
        {
            active_stack_index_ = 0;
            return;
        }

        if (active_stack_index_ >= call_stack_desc_.size())
            active_stack_index_ = call_stack_desc_.size() - 1;
    }

    // Инициализация объекта - приведение его в начальное состояние и сброс всех данных, кроме звонкового функтора.
    void DebugContext::Clear()
    {
        #ifndef MYTHON_UNITHREAD
            std::scoped_lock sc_lock(breakpoints_mutex_, call_stack_desc_mutex_);
        #endif

        SimpleContext::Clear();
        call_stack_desc_.clear();
        breakpoints_.clear();
        active_stack_index_ = 0;
        debug_exec_ = DebugExecutionMode::DEBUG_NO_DEBUG;
    }

    // Выполнение операций над списком точек останова.
    // Расчёт количества существующих к данному моменту точек останова.
    size_t DebugContext::BreakpointsCount(bool is_enabled_only) const
    { // Получение общего количества (или только активных при is_enabled_only == true) существующих бряков.
        #ifndef MYTHON_UNITHREAD
            std::lock_guard lg(breakpoints_mutex_);
        #endif

        size_t result = 0;
        for (const BreakpointDesc& break_desc : breakpoints_)
        {
            if (break_desc.position.IsValid())
            {
                if (break_desc.is_enabled || !is_enabled_only)
                    ++result;
            }
        }
        return result;
    }

    size_t DebugContext::MaxBreakpointIndex() const
    { // Возвращает максимальный индекс существующей точки останова. Если бряков вовсе нет, возвращаем зарезервированное значение.
        #ifndef MYTHON_UNITHREAD
            std::lock_guard lg(breakpoints_mutex_);
        #endif

        size_t result = RESERVED_VALUE;
        for (size_t break_index = 0;
             break_index < breakpoints_.size() && breakpoints_[break_index].position.IsValid();
             ++break_index)
        {
            if (result == RESERVED_VALUE || result < break_index)
                result = break_index;
        }
        return result;
    }

    size_t DebugContext::AddBreakpoint(const BreakpointDesc& new_break)
    { // Создание полностью описанной точки останова.
        #ifndef MYTHON_UNITHREAD
            std::lock_guard lg(breakpoints_mutex_);
        #endif

        size_t break_index = 0;
        for (; break_index < breakpoints_.size() && breakpoints_[break_index].position.IsValid(); ++break_index);
        if (break_index >= breakpoints_.size()) // Пустых ячеек в breakpoints_ сейчас нет, создаём новые.
            breakpoints_.resize(breakpoints_.size() + 10);

        breakpoints_[break_index] = new_break;
        return break_index;
    }

    size_t DebugContext::AddBreakpoint(const ProgramCommandDescriptor& new_break_position)
    { // Создание типового бряка. Все его параметры, кроме положения, принимаются по умолчанию.
        return AddBreakpoint({.position = new_break_position});
    }

    // Создание бряка на любой вызов некоторого метода с именем methon_name, принимающий params_count аргументов и принадлежащий
    // классу class_name (или любому классу, если class_name пуст).
    size_t DebugContext::AddBreakAtMethod(const std::string& method_name, size_t params_count, const std::string& class_name)
    {
        ast::ProgramCompound* program_root = dynamic_cast<ast::ProgramCompound*>(GetProgramRoot());
        if (!program_root)
        {
            assert(false);
            return RESERVED_VALUE;
        }

        if (ProgramCommandDescriptor method_def_pos = TypeTraitsInstance::ScanForMethod
            (program_root->GetDeclaredClassesDef(), MangleMethodFunctionName(method_name, params_count));
            method_def_pos != DUMB_PROG_POS)
            // Метод с затребованной сигнатурой найден. Создаёи бряк на его декларацию и возвращаем индекс этого бряка.
            return AddBreakpoint(method_def_pos);
        else    // Метод с указанными именными характеристиками и классовой принадлежностью найти не удалось.
            return RESERVED_VALUE;
    }

    // Создание бряка на любой вызов свободной функции с именем free_func_name, принимающей params_count аргументов.
    size_t DebugContext::AddBreakAtFreeFunction(const std::string& free_func_name, size_t params_count)
    {
        ast::ProgramCompound* program_root = dynamic_cast<ast::ProgramCompound*>(GetProgramRoot());
        if (!program_root)
        {
            assert(false);
            return RESERVED_VALUE;
        }

        if (ProgramCommandDescriptor free_func_def_pos = TypeTraitsInstance::ScanForFreeFunction
            (program_root->GetDeclaredFreeFunctionsDef(), MangleMethodFunctionName(free_func_name, params_count));
            free_func_def_pos != DUMB_PROG_POS)
            // Свободная функция с затребованной сигнатурой найдена. Создаёи бряк на её декларацию и возвращаем его индекс.
            return AddBreakpoint(free_func_def_pos);
        else    // Свободную функцию с указанными именными характеристиками найти не удалось.
            return RESERVED_VALUE;
    }

    bool DebugContext::DeleteBreakpoint(size_t breakpoint_index)
    { // Удаление существующей в списке точки останова.
        CHECK_RET_BREAKPOINT(false);
        breakpoints_[breakpoint_index].position = DUMB_PROG_POS;
        breakpoints_[breakpoint_index].is_enabled = false;
        return true;
    }

    bool DebugContext::EnableBreakpoint(size_t breakpoint_index, bool is_enabled)
    { // Активация (включение) точки останова с индексом breakpoint_index.
        CHECK_RET_BREAKPOINT(false);
        breakpoints_[breakpoint_index].is_enabled = is_enabled;
        return true;
    }

    void DebugContext::DisableBreakpoint(size_t breakpoint_index)
    { // Активация (включение) точки останова с индексом breakpoint_index.
        EnableBreakpoint(breakpoint_index, false);
    }

    bool DebugContext::IsBreakpointEnabled(size_t breakpoint_index) const
    { // Получение состояния "активности" точки останова с индексом breakpoint_index.
        CHECK_RET_BREAKPOINT(false);
        return breakpoints_[breakpoint_index].is_enabled;
    }

    bool DebugContext::MakeBreakpointConditional(size_t breakpoint_index, bool is_conditional)
    { // Присоединение условия к точке останова с индексом breakpoint_index.
        CHECK_RET_BREAKPOINT(false);
        breakpoints_[breakpoint_index].is_conditional = is_conditional;
        return true;
    }

    void DebugContext::MakeBreakpointUnconditional(size_t breakpoint_index)
    { // Снятие условия с точки останова с индексом breakpoint_index.
        MakeBreakpointConditional(breakpoint_index, false);
    }

    bool DebugContext::IsBreakpointConditional(size_t breakpoint_index) const
    { // Получение флага наличия связанного условия для точки останова с индексом breakpoint_index.
        CHECK_RET_BREAKPOINT(false);
        return breakpoints_[breakpoint_index].is_conditional;
    }

    int DebugContext::GetBreakpointCount(size_t breakpoint_index) const
    { // Получение счетчика срабатываний для бряка с индексом breakpoint_index.
        CHECK_RET_BREAKPOINT(-1);
        return breakpoints_[breakpoint_index].break_count;
    }

    bool DebugContext::SetBreakpointCount(size_t breakpoint_index, int new_break_count)
    { // Установка счетчика срабатываний для бряка с индексом breakpoint_index в значение new_break_count.
        CHECK_RET_BREAKPOINT(false);
        breakpoints_[breakpoint_index].break_count = new_break_count;
        return true;
    }

    BreakpointDesc DebugContext::GetBreakpointDesc(size_t breakpoint_index) const
    { // Возвращает копию полного описания точки останова с индексом breakpoint_index.
        CHECK_RET_BREAKPOINT(DUMB_BREAKPOINT);
        return breakpoints_[breakpoint_index];
    }

    size_t DebugContext::FindBreakpoints(const ProgramCommandDescriptor& test_break_position, Executable* exec_statement, Closure& closure)
    { // Составление списка точек останова для исходной строки test_break_position, которые должны сработать в данный момент.
        #ifndef MYTHON_UNITHREAD
            std::lock_guard lg(breakpoints_mutex_);
        #endif

        triggered_breakpoints_.clear();
        size_t break_index = 0;
        for (; break_index < breakpoints_.size(); ++break_index)
        {
            BreakpointDesc& break_desc = breakpoints_[break_index];
            if (break_desc.position.IsValid() && break_desc.position == test_break_position && break_desc.is_enabled)
            { // Перебираем все существующие и активные точки останова, установленные на test_break_position.
                if (break_desc.is_conditional)
                { // Это условная точка останова, нужно проверить выполнение условия, что мы сейчас и проделаем.
                    if (!debug_callback_)
                        // При отсутствии обработчика звонков (который и должен произвести проверку) будем считать условие бряка невыполненным.
                        continue;

                    // Расширяем таблицу символов дополнительной переменной с номером (индексом) проверяемого бряка.
                    closure[BREAKPOINT_INFO_FIELD_NAME] = ObjectHolder::Own(runtime::Number(static_cast<int>(break_index)));
                    DebugExecutionMode break_test_result =
                        debug_callback_(DebugCallbackReason::DEBUG_CALLBACK_CHECK_CONDITION, exec_statement, closure, *this);
                    closure.erase(BREAKPOINT_INFO_FIELD_NAME); // Удаляем ранее созданную временную переменную.

                    if (break_test_result == DebugExecutionMode::DEBUG_NO_DEBUG)
                        // Проверка связанного условия для условной точки останова дала отрицательный результат - такую точку пропускаем.
                        continue;
                }
                // Очередная нужная точка останова с индексом break_index найдена. Добавим её в формируемый список.
                triggered_breakpoints_.push_back(break_index);
            }
        }
        return triggered_breakpoints_.size();
    }

    std::vector<size_t> DebugContext::GetTriggredBreakpoints() const
    {
        #ifndef MYTHON_UNITHREAD
            std::lock_guard lg(breakpoints_mutex_);
        #endif

        return triggered_breakpoints_;
    }

    size_t DebugContext::GetTriggredBreakpointsCount() const
    {
        #ifndef MYTHON_UNITHREAD
            std::lock_guard lg(breakpoints_mutex_);
        #endif

        return triggered_breakpoints_.size();
    }

    // Запоминание указателя на первую инструкцию (узла АСД любого типа) в очередной строке исходника.
    void DebugContext::SetFirstProperStatementInRow(Executable* exec_obj_ptr)
    {
        first_row_exec_obj_ = exec_obj_ptr;
    }

    // Возвращение ранее сохранённого указателя на первую инструкцию (узел АСД) текущей строки исходника.
    Executable* DebugContext::GetFirstProperStatementInRow()
    {
        return first_row_exec_obj_;
    }

    // Сброс всех строковых признаков ранее совершённых отладочных звонков.
    void DebugContext::ClearAllRowCallbackFlags()
    {
        row_callback_flags_ = CallbackCategoryFlag::CALLBACK_CAT_NOTHING;
    }

    // Установка строкового (для текущей строки исходника) флага совершённого отладочного звонка категории set_flag.
    #ifndef MYTHON_UNITHREAD
        DebugContext::CallbackCategoryFlag DebugContext::SetRowCallbackFlag(CallbackCategoryFlag set_flag)
        {
            CallbackCategoryFlag old_callback_flags_value = row_callback_flags_.load(), new_callback_flags_value;

            do
            {
                new_callback_flags_value = static_cast<CallbackCategoryFlag>(old_callback_flags_value | set_flag);
            }
            while (!row_callback_flags_.compare_exchange_strong(old_callback_flags_value, new_callback_flags_value));

            return new_callback_flags_value;
        }
    #else
        DebugContext::CallbackCategoryFlag DebugContext::SetRowCallbackFlag(CallbackCategoryFlag set_flag)
        {
            return row_callback_flags_ = static_cast<CallbackCategoryFlag>(callback_flags_ | set_flag);
        }
    #endif

    // Возврат текущего состояния строковых флагов совершённых ранее звонков.
    DebugContext::CallbackCategoryFlag DebugContext::GetRowCallbackFlags() const
    {
        return row_callback_flags_;
    }
} // namespace runtime

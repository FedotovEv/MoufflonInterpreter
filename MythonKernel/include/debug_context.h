#pragma once

#include "declares.h"
#include "runtime.h"
#include <climits>
#include <mutex>

namespace runtime
{
    struct CallStackEntry
    {
        ProgramCommandDescriptor call_command; // Строка исходника, в которой находится
                                               // точка вызова метода, создавшего данный стековый кадр.
        ProgramCommandDescriptor first_command; // Строка первой исполняемой команды данного стекового кадра
        Closure* closure_ptr = nullptr; // Указатель на таблицу символов стекового кадра (таблицу символов
                                        // его головного метода).
        std::string info_data;  // Имя стекового кадра (например, имя метода, которому этот кадр принадлежит).
        bool is_valid = false;  // Флаг корректности всех полей структуры. Если равен "ЛОЖЬ", формирование записи ещё не завершено.
    };

    struct BreakpointDesc
    {
        static constexpr ProgramCommandDescriptor DUMB_PROG_POS{.module_id = -1, .module_string_number = -1};

        ProgramCommandDescriptor position{-1, -1};  // Положение точки останова в исходном коде МУФЛОН-программы.
        int break_count = 0;            // Счетчик срабатываний этой точки.
        bool is_conditional = false;    // Является ли точка останова условной. Если == "ИСТИНА", к точке привязано некоторое условие, которое будет
                                        // предварительно проверено (перед срабатыванием бряка).
        bool is_enabled = true;         // Флаг постоянного включения/отключения точки останова. Если == "ИСТИНА", бряк активен и может сработать.
        //bool is_passed = false;         // Флаг временного отключения точки останова. Если == "ИСТИНА", этот бряк считается отключённым и при поиске будет пропущен.
    };

    // Код причины приостановки исполняемой программы и срабатывания отладочного звонка.
    enum class DebugCallbackReason
    {
        DEBUG_CALLBACK_UNKNOWN = 0,
        DEBUG_CALLBACK_INIT,                // Инициализирующий звонок в процессе обработки инструкции ProgramCompound.
        DEBUG_CALLBACK_STEP_IN,             // Завершение шага с (возможным) заходом в метод или функцию.
        DEBUG_CALLBACK_STEP_OUT,            // Завершение шага с обходом возможных вызовов методов или функций.
        DEBUG_CALLBACK_EXIT_METHOD,         // Выход (завершение) из метода.
        DEBUG_CALLBACK_BREAKPOINT,          // Звонок по поводу срабатывания точки останова программы.
        DEBUG_CALLBACK_CHECK_CONDITION,     // Звонок проверки выполнения сопряжённого условия для некоторого бряка.
        DEBUG_CALLBACK_STD_MAX = DEBUG_CALLBACK_CHECK_CONDITION,    // Максимальное стандартное значение члена перечисления.
        DEBUG_CALLBACK_USER = 1024          // Минимальная величина пользовательской области значений.
    };

    // Перечисление, кодирующее режим дальнейшего исполнения программы при её возобновлении после отладочной приостановки.
    enum class DebugExecutionMode
    {
        DEBUG_NO_DEBUG = 0,     // Режим без отладки.
        DEBUG_SIMPLE_RUN,       // Запуск под отладчиком до какой-либо контрольной точки.
        DEBUG_STEP_IN,          // Исполнение до начала следующей строки исходника.
        DEBUG_STEP_OUT,         // Исполнение до начала следующей строки исходника, обходя все вызовы функций.
        DEBUG_EXIT_METHOD       // Запуск вплоть до оператора выхода из текущей функции.
    };

    using DebugCallback =
        std::function<DebugExecutionMode(DebugCallbackReason, Executable*, Closure&, Context&)>;

    class DebugContext : public SimpleContext
    {
    public:
        static constexpr size_t RESERVED_VALUE = (std::numeric_limits<size_t>::max)();

        explicit DebugContext(std::ostream& output, DebugCallback debug_callback, LinkageFunction external_link = LinkageFunction())
            : SimpleContext(output, std::move(external_link)), debug_callback_(std::move(debug_callback))
        {}

        // Получение функтора-перехватчика отладочного звонка, назначенного при конструировании контекста.
        const DebugCallback& GetDebugCallback() const;
        // Инициализация объекта - приведение его в начальное состояние и сброс всех данных, кроме звонкового функтора.
        void Clear();

        // Три определённые ниже функции нарушают инкапсуляцию класса и синхронизацию при параллельном доступе,
        // поэтому предназначены только для отладочных и исследовательских целей.
        // Возврат просмотровой ссылки на стек вызовов исполяющейся МУФЛОН-программы.
        const std::vector<CallStackEntry>& GetCallStack() const;
        // Получение просмотровой ссылки на список отладочных точек останова, определённых в данном контексте (как реально существующих,
        // так и вакантных мест, образовавшихся после удаления ранее существовавшего бряка).
        const std::vector<BreakpointDesc>& GetBreakpointsList() const;
        // Переназначение (установка нового) обработчика отладочных стоп-звонков.
        void SetDebugCallback(DebugCallback debug_callback);

        // Функции-члены получения/установки режима возобновления (исполнения) программы.
        // Извлекатель текущего режима исполнения отлаживаемой программы.
        DebugExecutionMode GetDebugMode() const;
        // Установка нового значения режима исполнения программы под отладчиком.
        void SetDebugMode(DebugExecutionMode new_debug_exec);

        // Функции-члены для работы с моделью стека вызовов свободных функций/методов отлаживаемой программы.
        // Количество записей в стеке вызовов на данный момент времени.
        size_t GetCallStackSize() const;
        // Очистка стека вызовов - приведение его в исходное состояние на момент запуска программы для отладки.
        void ClearCallStack();
        // Формирование/правка/удаление описателя отладочного стекового кадра метода или функции отлаживаемой программы.
        // Добавление нового стекового кадра на верхушку стека.
        size_t PushCallStackEntry(const CallStackEntry& new_stack_entry);
        // Обновление стекового кадра с индексом stack_entry_index в соответствии с данными аргумента new_stack_entry.
        // При неуказании номера записи(вызове со значением stack_entry_index по умолчанию) будет обновлена запись в верхушке стека.
        size_t UpdateCallStackEntry(const CallStackEntry& new_stack_entry, size_t stack_entry_index = RESERVED_VALUE);
        // Извлечение из исполнительского стека записи с индексом stack_entry_index. При неуказании номера записи (вызове со
        // значением stack_entry_index по умолчанию) будет извлечена запись из верхушки стека.
        std::pair<CallStackEntry, size_t> GetCallStackEntry(size_t stack_entry_index = RESERVED_VALUE) const;
        // Выталкивание из стека его верхнего элемента, который также будт возвращён как результат работы данной функции.
        CallStackEntry PopCallStackEntry();
        // Проверка полной сформированности стекового кадра с индексом stack_entry_index (или на вершине стека вызовов,
        // если stack_entry_index задан по умолчанию).
        bool IsCallStackEntryValid(size_t stack_entry_index = RESERVED_VALUE) const;

        // Методы обслуживания указателя (индекса) положения в стеке вызовов текущей отлаживаемой функции
        // (то есть той, с которой в данный момент работает отладчик).
        // Возврат положения (индекса) текущего "активного" стекового кадра (то есть кадра, принадлежащего тому методу или функции,
        // с которой в данный момент работает отладчик).
        size_t GetDebugStackCounter() const;
        // Установка нового значения индекса "активного" стекового кадра.
        void SetDebugStackCounter(size_t debug_exec_stack_counter);
        // Перемещение "активного" стекового кадра на единицу ближе к корню (используется при обработке выхода из функции при работе в
        // пошаговом режиме под отладчиком).
        size_t DecDebugStackCounter();

        // Выполнение операций над списком точек останова.
        // Подсчёт общего количества существующих на данный момент точек останова (бряков).
        size_t BreakpointsCount(bool is_enabled_only = false) const;
        // Возвращает максимальный индекс существующей точки останова. Если бряков вовсе нет, возвращаем зарезервированное значение.
        size_t MaxBreakpointIndex() const;
        // Создание полностью описанной точки останова.
        size_t AddBreakpoint(const BreakpointDesc& new_break);
        // Создание типового бряка. Все его параметры, кроме положения, принимаются по умолчанию.
        size_t AddBreakpoint(const ProgramCommandDescriptor& new_break_position);
        // Удаление существующей в списке точки останова.
        bool DeleteBreakpoint(size_t breakpoint_index);
        // Активация (включение) точки останова с индексом breakpoint_index.
        bool EnableBreakpoint(size_t breakpoint_index, bool is_enabled = true);
        // Активация (включение) точки останова с индексом breakpoint_index.
        void DisableBreakpoint(size_t breakpoint_index);
        // Получение состояния "активности" точки останова с индексом breakpoint_index.
        bool IsBreakpointEnabled(size_t breakpoint_index) const;
        // Присоединение условия к точке останова с индексом breakpoint_index.
        bool MakeBreakpointConditional(size_t breakpoint_index, bool is_conditional = true);
        // Снятие условия с точки останова с индексом breakpoint_index.
        void MakeBreakpointUnconditional(size_t breakpoint_index);
        // Получение флага наличия связанного условия для точки останова с индексом breakpoint_index.
        bool IsBreakpointConditional(size_t breakpoint_index) const;
        // Получение счетчика срабатываний для бряка с индексом breakpoint_index.
        int GetBreakpointCount(size_t breakpoint_index) const;
        // Установка счетчика срабатываний для бряка с индексом breakpoint_index в значение new_break_count.
        bool SetBreakpointCount(size_t breakpoint_index, int new_break_count = 0);
        // Получение копии полного описания существующей точки останова с индексом breakpoint_index.
        BreakpointDesc GetBreakpointDesc(size_t breakpoint_index) const;
        // Формирование списка точек останова для исходной строки test_break_position, которые должны сработать в данный момент.
        std::vector<size_t> FindBreakpoints(const ProgramCommandDescriptor& test_break_position, Executable* exec_statement, Closure& closure);

    private:
        static constexpr BreakpointDesc DUMB_BREAKPOINT{.position = {-1, -1}, .break_count = 0, .is_conditional = false, .is_enabled = false/*, .is_passed = true*/};

        #ifndef MYTHON_UNITHREAD
            // Многопоточный вариант отладочного контекста, предназначенный для одновременного доступа из нескольких параллельных
            // потоков (исполняющего и отлаживающего). Этот вариант класса DebugContext содержит некоторые элементы потокобезопасности, так как
            // предназначен для одновременного доступа как из потока исполнения программы, так и из параллельно выполняющегося потока стороннего отладчика.
            DebugCallback debug_callback_; // Экземпляр функтора-обработчика отладочных звонков.
            // Текущий режим исполнения программы.
            std::atomic<DebugExecutionMode> debug_exec_{DebugExecutionMode::DEBUG_NO_DEBUG};
            // Список существующих точек останова.
            mutable std::mutex breakpoints_mutex_;
            std::vector<BreakpointDesc> breakpoints_;
            // Описание текущего стека вызовов программы.
            mutable std::mutex call_stack_desc_mutex_;
            std::vector<CallStackEntry> call_stack_desc_;
            // debug_exec_stack_counter_ - индекс стекового кадра, принадлежащего методу (или свободной функции), с которой в данный
            // момент проводится работа (произведён шаг или запуск программы после последнего состоявшегося отладочного звонка).
            std::atomic<size_t> debug_exec_stack_counter_ = 0;
        #else
            // Однопоточный вариант отладочного контекста. Доступ к его методам может выполняться только последовательно, с применением
            // способов внешней синхронизации.
            DebugCallback debug_callback_; // Экземпляр функтора-обработчика отладочных звонков.
            // Текущий режим исполнения программы.
            DebugExecutionMode debug_exec_{DebugExecutionMode::DEBUG_NO_DEBUG};
            // Список существующих точек останова.
            std::vector<BreakpointDesc> breakpoints_;
            // Описание текущего стека вызовов программы.
            std::vector<CallStackEntry> call_stack_desc_;
            // debug_exec_stack_counter_ - индекс стекового кадра, принадлежащего методу (или свободной функции), с которой в данный
            // момент проводится работа (произведён шаг или запуск программы после последнего состоявшегося отладочного звонка).
            size_t debug_exec_stack_counter_ = 0;
        #endif
    };
    
    std::string CallbackReasonToString(DebugCallbackReason debug_callback_reason);
    std::string ExecutionModeToString(DebugExecutionMode debug_execution_mode);
    //
    std::ostream& operator<<(std::ostream& ostr, DebugCallbackReason debug_callback_reason);
    std::ostream& operator<<(std::ostream& ostr, DebugExecutionMode debug_execution_mode);
} // namespace runtime

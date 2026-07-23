#pragma once

// Итератор для словаря (ассоциативного массива).
class MapInstance;
class MapCursor : public Object
{
public:
    MapCursor(MapInstance& map_instance, std::map<std::string, ObjectHolder>& map_storage);
    void Print(std::ostream& os, Context& context) override;
    const void* GetPtr() const
    {
        return nullptr;
    }

    size_t SizeOf() const
    {
        return 0;
    }
    bool IsCursorValid();
    ObjectHolder CursorGetKey();
    ObjectHolder CursorGetValue();
    bool Begin();
    bool CursorLowerBound(const std::string& map_key);
    bool CursorNext();
    bool CursorPrevious();
    bool IsCursorEnd();
    bool IsCursorBegin();

private:
    MapInstance& map_instance_ref_;
    std::map<std::string, ObjectHolder>& map_storage_ref_;
    std::map<std::string, ObjectHolder>::iterator map_iterator_;
    int iterator_pack_serial_;
};

class ArrayInstance : public CommonClassInstance
{ // Экземпляр массива - специального встроенного объекта с предопределенным набором методов.
public:
    using ArrayCallMethod = ObjectHolder(ArrayInstance::*)(const std::string&, const std::vector<ObjectHolder>&,
                                                           Context&);
    ArrayInstance(std::vector<int> elements_count);
    void Print(std::ostream& os, Context& context) override;
    /*
     * Вызывает у объекта-массива метод method, передавая ему actual_args параметров.
     * Параметр context задаёт контекст для выполнения метода. Если метод method не относится к тем,
     * которые поддерживает массив, метод выбрасывает исключение runtime_error.
     * Набор методов, обеспечиваемых массивом, следующий:
     * get(... индексы ...) -
     *      - служит для считывания и установки значения элемента массива, определенного
     *      набором координат-индексов. Все индексы базируются к нулю (минимальный
     *      индекс элемента для каждой размерности равен 0).
     * get_array_dimensions() - получение количества измерений массива.
     * get_dimension_count(dimension_number) -
     *      - получение количества элементов для размерности dimension_number.
     *      Номер размерности базируется к 1 (младший индекс и
     *      соответствующая размерность имеют номер 1).
     * resize(... количество элементов по размерностям ...) -
     *      - пересоздание массива с иной размерностью и количеством элементов.
     * clear() - очистка массива. Для одномерных массивов размер сбрасывается в нуль (массив опустошается),
     *         для многомерного массива все его элементы устанавливаются в None.
     * Следующие методы определены только для одномерных массивов. Для многомерных массивов будет выброшено
     * исключение runtime_error.
     * push_back(new_element) - добавляет элемент new_element в конец массива.
     * back() - позволяет считать или установить последний элемент массива.
     * pop_back() - удаляет из массива последний элемент
     */
    ObjectHolder Call(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                      Context& context, const std::string& parent_name = {}) override;
    bool HasMethod(const std::string& method_name, size_t argument_count, const std::string& parent_name = {}) const override;

    [[nodiscard]] std::string GetClassName(void) const override
    {
        return "array";
    }

private:
    static const std::unordered_map<std::string_view, ArrayCallMethod> array_method_table_;
    static const std::unordered_map<std::string_view, std::pair<size_t, size_t>> array_method_argument_count_;

    // Обработчики методов класса "массив"
    ObjectHolder MethodGet(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                           Context& context);
    ObjectHolder MethodGetArrayDimensions(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                          Context& context);
    ObjectHolder MethodGetDimensionCount(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                         Context& context);
    ObjectHolder MethodResize(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                              Context& context);
    ObjectHolder MethodClear(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                             Context& context);
    ObjectHolder MethodPushBack(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                Context& context);
    ObjectHolder MethodBack(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                            Context& context);
    ObjectHolder MethodPopBack(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                               Context& context);

    std::vector<int> elements_count_;
    std::vector<ObjectHolder> data_storage_;
};

class MapInstance : public CommonClassInstance
{ // Экземпляр ассоциативного массива (словаря) - специального встроенного объекта с предопределенным набором методов.
public:
    using MapCallMethod = ObjectHolder(MapInstance::*)(const std::string&, const std::vector<ObjectHolder>&,
                                                       Context&);
    MapInstance() = default;
    void Print(std::ostream& os, Context& context) override;
    /*
     * Вызывает у объекта-словаря метод method, передавая ему actual_args параметров.
     * Параметр context задаёт контекст для выполнения метода. Если метод method не относится к тем,
     * которые поддерживает словарь, метод выбрасывает исключение runtime_error.
     * Набор методов, обеспечиваемых массивом, следующий:
     * insert(key) - вставка в массив элемента с ключом key.
     * find(key) - чтение или изменение уже существующего элемента с ключом key.
     * erase(key) - удаление элемента с ключом key.
     * contains(key) - проверка наличия элемента с ключом key.
     * clear() - очищает словарь, удаляя его содержимое.
     * begin() - возврат "итератора", указывающего на первый элемент массива.
     * previous(iterator) - возвращает итератор, указывающий на элемент словаря, предшествующий iterator.
     * next(iterator) - возвращает итератор, указывающий на элемент словаря, следующий после iterator.
     * key(iterator) - возвращает ключ элемента, соответствующего iterator.
     * value(iterator) - чтение или изменение элемента, на который указывает iterator.
     * is_iterator_begin(iterator) - возврат "истины", если итератор указывает на первый элемент словаря.
     * is_iterator_end(iterator) -  возврат "истины", если итератор указывает _за_ последний элемент словаря.
     * release() - сообщает об окончании процесса перечисления элементов словаря
     */
    ObjectHolder Call(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                      Context& context, const std::string& parent_name = {}) override;
    bool HasMethod(const std::string& method_name, size_t argument_count, const std::string& parent_name = {}) const override;

    [[nodiscard]] std::string GetClassName(void) const override
    {
        return "map";
    }

    int AllocIteratorPackSerial()
    {
        if (!is_in_iterator_mode_)
            iterator_pack_serial_ = ++last_iterator_pack_serial_;

        is_in_iterator_mode_ = true;
        return iterator_pack_serial_;
    }

    int GetIteratorPackSerial()
    {
        return iterator_pack_serial_;
    }

    bool GetIteratorModeFlag()
    {
        return is_in_iterator_mode_;
    }

private:
    static const std::unordered_map<std::string_view, MapCallMethod> map_method_table_;
    static const std::unordered_map<std::string_view, std::pair<size_t, size_t>> map_method_argument_count_;

    // Обработчики методов класса "ассоциативный массив(словарь)"
    ObjectHolder MethodInsert(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                              Context& context);
    ObjectHolder MethodFind(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                            Context& context);
    ObjectHolder MethodErase(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                             Context& context);
    ObjectHolder MethodContains(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                Context& context);
    ObjectHolder MethodClear(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                             Context& context);                                
    ObjectHolder MethodBegin(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                             Context& context);
    ObjectHolder MethodPrevious(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                Context& context);
    ObjectHolder MethodNext(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                            Context& context);
    ObjectHolder MethodKey(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                           Context& context);
    ObjectHolder MethodValue(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                             Context& context);
    ObjectHolder MethodIsCursorBegin(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                       Context& context);
    ObjectHolder MethodIsCursorEnd(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                     Context& context);
    ObjectHolder MethodRelease(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                               Context& context);

    std::map<std::string, ObjectHolder> map_storage_;
    bool is_in_iterator_mode_ = false;
    int iterator_pack_serial_;

    static int last_iterator_pack_serial_;
};

class CoroutineInstance : public CommonClassInstance
{
public:
    using CoroutineCallMethod = ObjectHolder(CoroutineInstance::*)(const std::string&, const std::vector<ObjectHolder>&,
                                                                   Context&);
    CoroutineInstance(ClassInstance* class_instance, const runtime::Method* method, Closure& closure);
    CoroutineInstance(FreeFunction* free_function, Closure& closure);
    CoroutineInstance(const CoroutineInstance&) = delete;
    CoroutineInstance(CoroutineInstance&&) = default;
    CoroutineInstance& operator=(const CoroutineInstance&) = delete;
    CoroutineInstance& operator=(CoroutineInstance&&) = default;

    void Print(std::ostream& os, Context& context) override;
    /*
     * Вызывает у объекта-состояния сопрограммы метод method, передавая ему actual_args параметров.
     * Параметр context задаёт контекст для выполнения метода. Если метод method не относится к тем,
     * которые поддерживает массив, метод выбрасывает исключение runtime_error.
     * Набор методов, обеспечиваемых объектом-статусом исполнения сопрограммы, следующий:
     resume() - возобновляет сопрограмму с её последней точки приостановки.
     is_started() - возвращает "ИСТИНУ", если сопрограмма хотя бы один раз запускалась (возобновлялась)
                    после создания данного объекта (то есть после ее инициализирующего вызова).
     is_awaiting() - возвращает "ИСТИНУ", если сопрограмма приостановлена (не завершена) и в настоящий момент
                     ее работу можно возобновить вызовом resume() повторно.
     value() - результат последнего вызова сопрограммы.     
     */
    ObjectHolder Call(const std::string& method_name, const std::vector<ObjectHolder>& actual_args,
                      Context& context, const std::string& parent_name = {}) override;
    bool HasMethod(const std::string& method_name, size_t argument_count, const std::string& parent_name = {}) const override;

    [[nodiscard]] std::string GetClassName(void) const override
    {
        return "coroutine";
    }

    void SuspendCoroutine(runtime::CoroutineSuspendType suspend_type)
    {
        is_awaiting_ = true;
        suspend_type_ = suspend_type;
    }

    // Переадресующие вызовы для методов поля workflow_.
    WorkflowPosition* PushBack(WorkflowPosition new_workflow_position)
    {
        return workflow_.PushBack(std::move(new_workflow_position));
    }

    WorkflowPosition PopBack(WorkflowPosition::WorkPosType find_pos_type = WorkflowPosition::WorkPosType::WORK_POS_UNKNOWN)
    {
        return workflow_.PopBack(find_pos_type);
    }

    WorkflowPosition* Current()
    {
        return workflow_.Current();
    }

    WorkflowPosition* SetIndex(int new_index = 0)
    {
        return workflow_.SetIndex(new_index);
    }

    WorkflowPosition* Advance(int dist = 1)
    {
        return workflow_.Advance(dist);
    }

    WorkflowPosition* Back()
    {
        return workflow_.Back();
    }

    // Установщики/извлекатели некоторых других полей состояния сопрограммы.
    runtime::ClassInstance* GetLastAwaitable()
    {
        return last_awaitable_instance_;
    }

    void SetLastAwaitable(runtime::ClassInstance* last_awaitable_instance)
    {
        last_awaitable_instance_ = last_awaitable_instance;
    }

    ObjectHolder GetLastAwaitSuspendValue()
    {
        return last_await_suspend_value_;
    }

    void SetLastAwaitSuspendValue(ObjectHolder last_await_suspend_value)
    {
        last_await_suspend_value_ = std::move(last_await_suspend_value);
    }

private:
    static const std::unordered_map<std::string_view, CoroutineCallMethod> coroutine_method_table_;
    static const std::unordered_map<std::string_view, std::pair<size_t, size_t>> coroutine_method_argument_count_;

    // Указатель free_function_ заполняется, если сопрограмма является свободной функцией. В этом случае class_instance_ == nullptr.
    // Если же она построена на основе метода класса, то free_function_ == nullptr, а class_instance_ != nullptr.
    FreeFunction* free_function_ = nullptr;
    // Указатель на экземпляр класса, которому принадлежит (ему или его предкам) метод-сопрограмма.
    ClassInstance* class_instance_ = nullptr;
    const Method* method_ = nullptr;    // Указатель на сам метод-сопрограмму (его дескриптор).
    
    // Поля состояния сопрограммы, отражающие ее статус в состоянии приостановки или после окончательного завершения.
    Closure      coro_closure_;         // Здесь будет сохраняться символьная таблица сопрограммы при её приостановке.
    bool         is_started_ = false;   // Признак сопрограммы, которая уже работала хотя бы единожды.
    bool         is_awaiting_ = false;  // Признак, что работа сопрограммы именно приостановлена, а не полностью завершена.
    // Тип точки приостановки сопрограммы (вид оператора, породившего эту точку).
    runtime::CoroutineSuspendType suspend_type_ = runtime::CoroutineSuspendType::SUSPEND_POINT_UNKNOWN;
    ObjectHolder ret_value_;            // Значение, возвращённое сопрограммой при крайнем сеансе её работы.
    // Поля, хранящие информацию, специфическую для работы инструкции co_await.
    // Указатель на объект-ждун, который использовался при последней приостановке сопрограммы оператором co_await.
    runtime::ClassInstance* last_awaitable_instance_ = nullptr;
    // Значение, возвращённое "стопором" (AwaitSuspend()) ждуна перед последней приостановкой сопрограммы оператором co_await.
    ObjectHolder last_await_suspend_value_;
    
    // Поля со сведениями о положении точки в потоке управления сопрограммы.
    WorkflowStackSaver last_workflow_;  // Состояние потока управления (положение исполняемой точки) в момент завершения
                                        // сопрограммы (исполнения операторов co_await, co_yield или co_yield_ref).
    WorkflowStackSaver workflow_;       // Текущее положение потока управления.
    ObjectHolder coro_awaitable_;       // Объект-ждун, назначенный данной сопрограмме.
    
    // Обработчики методов класса сопрограммы.
    ObjectHolder MethodResume(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodIsStarted(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodIsAwaiting(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodValue(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodGetAwaitable(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodSetAwaitable(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodSuspendType(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodIsFreeFunction(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
};

class TypeTraitsInstance : public CommonClassInstance
{
public:
    using TypeTraitsCallMethod = ObjectHolder(TypeTraitsInstance::*)(const std::string&, const std::vector<ObjectHolder>&, Context&);
    TypeTraitsInstance(ObjectHolder traits_value);
    TypeTraitsInstance(const TypeTraitsInstance&) = delete;
    TypeTraitsInstance(TypeTraitsInstance&&) = default;
    TypeTraitsInstance& operator=(const TypeTraitsInstance&) = delete;
    TypeTraitsInstance& operator=(TypeTraitsInstance&&) = default;

    void Print(std::ostream& os, Context& context) override;
    /*
     * Вызывает у объекта-состояния сопрограммы метод method, передавая ему actual_args параметров.
     * Параметр context задаёт контекст для выполнения метода. Если метод method не относится к тем,
     * которые поддерживает массив, метод выбрасывает исключение runtime_error.
     * 
     * Набор методов, обеспечиваемых объектом типового отпечатка, следующий:
        IsBool();                           // Возвращает "ИСТИНУ", если выражение имеет встроенный логический тип.
        IsNumeric();                        // Возвращает "ИСТИНУ", если выражение имеет встроенный числовой тип.
        IsString();                         // Возвращает "ИСТИНУ", если выражение имеет встроенный строковый тип.
        IsNone();                           // Возвращает "ИСТИНУ", если выражение относится к типу None (пустое).
        IsSameType(other);                  // Возвращает "ИСТИНУ", если выражение имеет тот же тип, что и аргумент метода.
        IsSameTarget(other);                // Возвращает "ИСТИНУ", если выражение ссылается на ту же область памяти, что и аргумент.
        IsClass(type_name);                 // Возвращает "ИСТИНУ", если имя типа выражения совпадает с аргументом метода.
        IsSuсcessorOf(other);               // Возвращает "ИСТИНУ", если тип выражения является наследником типа аргумента.
        IsPredecessorOf(other);             // Возвращает "ИСТИНУ", если тип выражения является предшественником типа аргумента.        
        IsSuсcessorOfName(type_name);       // Возвращает "ИСТИНУ", если тип выражения является наследником типа с именем, указываемым аргументом.        
        IsPredecessorOfName(type_name);     // Возвращает "ИСТИНУ", если тип выражения является предшественником типа с именем, указываемым аргументом.
        Id();                               // Возвращает уникальный целочисленный идент типа.
        Name();                             // Возвращает символьное имя типа.
        HasMethod(method_name, params_count);   // Возвращает "ИСТИНУ", если класс (к которому относится выражение) имеет метод, сигнатура
                                                // которого задаётся аргументами метода (их два - имя метода и количество его формальных параметров).
        HasField(field_name);               // Возвращает "ИСТИНУ", если класс (к которому относится выражение) имеет поле с именем, равным аргументу матода.
     */
    ObjectHolder Call(const std::string& method_name, const std::vector<ObjectHolder>& actual_args,
                      Context& context, const std::string& parent_name = {}) override;
    bool HasMethod(const std::string& method_name, size_t argument_count, const std::string& parent_name = {}) const override;

    [[nodiscard]] std::string GetClassName(void) const override
    {
        return "TypeTraits";
    }

    static void ClearInternalClassIds()
    {
        internal_classes_ids_.clear();
    }
    
    static void AppendInternalClassId(const std::string& class_name, int class_id)
    {
        internal_classes_ids_.emplace(std::pair{class_name, class_id});
    }

    static void ClearDeclaredClassDefs()
    {
        declared_classes_def_.clear();
    }
    
    static void AppendDeclaredClassDef(const std::string& class_name, ast::ClassDefinition* class_def);

private:
    static const std::unordered_map<std::string_view, TypeTraitsCallMethod> type_traits_method_table_;
    static const std::unordered_map<std::string_view, std::pair<size_t, size_t>> type_traits_method_argument_count_;
    // Словарь хранения идентов встроенных фиксированных классов инсполнительской среды.
    static std::unordered_map<std::string, int> internal_classes_ids_;
    // Словарь связи имени класса и его объекта-дескриптора типа ClassDefinition.
    static std::unordered_map<std::string, ast::ClassDefinition*> declared_classes_def_;

    ObjectHolder traits_value_;       // Характеризуемое значение.

    static int ObjectIdInternal(const ObjectHolder& what_id);
    static std::string ObjectNameInternal(const ObjectHolder& what_id);

    // Обработчики методов характеристического класса.
    ObjectHolder MethodIsBool(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodIsNumeric(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodIsString(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodIsNone(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodIsSameType(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodIsSameTarget(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodIsClass(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodIsSuсcessorOf(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodIsPredecessorOf(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodIsSuсcessorOfName(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodIsPredecessorOfName(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodId(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodName(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodHasMethod(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodHasField(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    // Методы извлечение-установки значений полей объекта по их строковым именам (именам в виде текстовой строки).
    ObjectHolder MethodGetFieldValue(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodSetFieldValue(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    // Метод вызова метода того объекта, для которого создана данная характеристика, по его строковому имени.
    ObjectHolder MethodCallMethod(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
};

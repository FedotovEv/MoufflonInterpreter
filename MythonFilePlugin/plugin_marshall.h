

#if defined (_WIN64) || defined(_WIN32)
    #define MYTHLON_INTERPRETER_EXPORT __declspec(dllexport)
    #define MYTHLON_MODULE_EXPORT __declspec(dllexport)
    #define MYTHLON_INTERPRETER_IMPORT __declspec(dllimport)
    #define MYTHLON_MODULE_IMPORT __declspec(dllimport)
#else
    #define MYTHLON_INTERPRETER_EXPORT
    #define MYTHLON_MODULE_EXPORT
    #define MYTHLON_INTERPRETER_IMPORT
    #define MYTHLON_MODULE_IMPORT
#endif

#pragma pack(push , 1)

enum ValueType
{
    // Примитивые скалярные значения, доступные непосредственно.
    VALUE_NOTHING = 0,
    VALUE_INTEGER,
    VALUE_DOUBLE,
    VALUE_STRING,
    VALUE_PAIR,
    // Комплексные значения, доступ к которым выполняется посредством специальных функций, экспортируемых ядром интерпретатора.
    VALUE_VECTOR,
    VALUE_CLOSURE,
    VALUE_CLASS_INSTANCE,
    VALUE_STATEMENT,
    VALUE_MAX_TYPE = VALUE_STATEMENT
};

// Идентификаторы компонент контекста исполнения программы.
enum ContextComponent
{
    CONTEXT_UNKNOWN = 0,
    CONTEXT_OSTREAM
};

struct StringValue
{
    int32_t     length = 0;
    char*       value = nullptr;
};

struct PrimitiveValue
{
    int32_t             value_type = 0;  // Одно из значений ValueType, допустимое для примитивных единичных типов.
    union
    {
        int32_t         int_value;
        double          double_value;
        StringValue     string_value;
    };
};

struct PairValue
{
    int32_t             value_type = VALUE_PAIR;  // Рамочный тип значения - пара VALUE_PAIR.
    PrimitiveValue      first;
    PrimitiveValue      second;
};

// Тип сложных составных объектов - вектора, таблицы символов, контекста (Context) исполнения программы, экземпляра класса, и. т. д.
struct ComplexValue
{
    int32_t     value_type = 0; // Одно из значений ValueType, примеимых для сложных типов.
    // Вспомогательное значение, указывающее, как правило, предельный индекс (точнее, значение, превышающее этот индекс на единицу),
    // с которым можно обращаться к интерфейсным методам, возвращающим истинное содержимое полей, структур и подструктур комплексного объекта.
    int32_t     length = 0;
    // Условный дескриптор объекта, содержимым которого сторона, формирующуя значение, может располагать по собственному усмотрению.
    void*       value = nullptr;
};

struct MethodInfo
{
    int32_t     class_name_length = 0;
    char*       class_name = nullptr;
    int32_t     method_name_length = 0;
    char*       method_name = nullptr;
    int32_t     params_count = -1;
};

#pragma pack(pop)

// Функции внешнего интерфейса, экспортируемые как ядром исполнительской среды МУФЛОНА, так и каждым предназначенным для
// работы с ним "втыкалом" - двоичным модулем расширения. Ядро применяет для реконструкции полученного объекта функции "втыкала", которое его
// сгенерировало, а втыкало, в свою очередь, обращается для той же цели к функциям ядра исполнительской среды.
extern "C"
{
    #ifdef MYTHLON_INTERPRETER_MODULE
        // Описание групп экспорта-импорта для динамической библиотеки "втыкалы".
        // Функции первой группы экспортируются втыкалой и импортируются ядром интерпретатора, а затем используются им для реконструкции объектов,
        // сгенерированных и возвращённых функциями втыкалы.
        // Определение типа значения с индексом index из состава комплекса complex. Тип - одно из значений перечисления ValueType.
        MYTHLON_MODULE_EXPORT int32_t GetModuleValueType(ComplexValue* complex, int32_t index, int32_t subindex);
        // Определение длины поля-приеминика для получения значения с индексом index из состава комплекса complex.
        MYTHLON_MODULE_EXPORT int32_t GetModuleValueLength(ComplexValue* complex, int32_t index, int32_t subindex);
        // Получение значения с индексом index из состава комплекса complex. Тело значения будет сохранено в область receiver (максимальна длина
        // записанных данных не превышает receiver_length байт).
        MYTHLON_MODULE_EXPORT void GetModuleValue(ComplexValue* complex, int32_t index, int32_t subindex, void* receiver, int32_t receiver_length);

        // Функции второй группы экспортируются втыкалой для получения информации (поиска) методов классов, 
        MYTHLON_MODULE_EXPORT int32_t HasMethod(MethodInfo* method_info);
        MYTHLON_MODULE_EXPORT void CallMethod(MethodInfo* method_info, void* receiver, int32_t receiver_length)


        // Функции второй группы экспортируются интерпретатором и импортируются модулем втыкалы для реконструкции (демаршализации) объектов,
        // поступивших во втыкалу из ядра исполнительской системы.
        // Определение типа значения с индексом index из состава комплекса complex. Тип - одно из значений перечисления ValueType.
        MYTHLON_MODULE_IMPORT uint32_t GetKernelValueType(ComplexValue* complex, uint32_t index, uint32_t subindex);
        // Определение длины поля-приеминика для получения значения с индексом index из состава комплекса complex.
        MYTHLON_MODULE_IMPORT uint32_t GetKernelValueLength(ComplexValue* complex, uint32_t index, uint32_t subindex);
        // Получение значения с индексом index из состава комплекса complex. Тело значения будет сохранено в область receiver (максимальна длина
        // записанных данных не превышает receiver_length байт).
        MYTHLON_MODULE_IMPORT void GetKernelValue(ComplexValue* complex, uint32_t index, uint32_t subindex, void* receiver, uint32_t receiver_length);
    
        // Наконец, функции третьей группы также экспортируются 
        // Выполнение операций по контексту исполнения программы. context_component указывает тип желаемой операции, context_op_argument - указатель
        // на ее аргумент (PrimitiveValue*, PairValue* или ComplexValue*).
        MYTHLON_MODULE_IMPORT ExecuteContextOp(ComplexValue* context_complex, uint32_t context_component, void* context_op_argument,
                                               void* result_receiver, uint32_t receiver_length);
        // Исполнение кода выполняемого программного объекта Statement (statement_complex). Сопровождающие аргументы исполнения - таблица символов closure_complex
        // и исполнительный контекст context_complex. Результат исполнения сохраняется в поле result_receiver длиной не более receiver_length.
        MYTHLON_MODULE_IMPORT ExecuteStatement(ComplexValue* statement_complex, ComplexValue* closure_complex, ComplexValue* context_complex,
                                               void* result_receiver, uint32_t receiver_length);
    #else
        // Описание групп экспорта-импорта для ядра интерпретатора МУФЛОНа.
        // Определение типа значения с индексом index из состава комплекса complex. Тип - одно из значений перечисления ValueType.
        MYTHLON_INTERPRETER_EXPORT uint32_t GetModuleValueType(ComplexValue* complex, uint32_t index, uint32_t subindex);
        // Определение длины поля-приеминика для получения значения с индексом index из состава комплекса complex.
        MYTHLON_INTERPRETER_EXPORT uint32_t GetModuleValueLength(ComplexValue* complex, uint32_t index, uint32_t subindex);
        // Получение значения с индексом index из состава комплекса complex. Тело значения будет сохранено в область receiver (максимальна длина
        // записанных данных не превышает receiver_length байт).
        MYTHLON_INTERPRETER_EXPORT void GetModuleValue(ComplexValue* complex, uint32_t index, uint32_t subindex, void* receiver, uint32_t receiver_length);
    
        // Выполнение операций по контексту исполнения программы. context_component указывает тип желаемой операции, context_op_argument - указатель
        // на ее аргумент (PrimitiveValue*, PairValue* или ComplexValue*).
        MYTHLON_INTERPRETER_EXPORT ExecuteContextOp(ComplexValue* context_complex, uint32_t context_component, void* context_op_argument,
                                                    void* result_receiver, uint32_t receiver_length);
        // Исполнение кода выполняемого программного объекта Statement (statement_complex). Сопровождающие аргументы исполнения - таблица символов closure_complex
        // и исполнительный контекст context_complex. Результат исполнения сохраняется в поле result_receiver длиной не более receiver_length.
        MYTHLON_INTERPRETER_EXPORT ExecuteStatement(ComplexValue* statement_complex, ComplexValue* closure_complex, ComplexValue* context_complex,
                                                    void* result_receiver, uint32_t receiver_length);
    #endif
}


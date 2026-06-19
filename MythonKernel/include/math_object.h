
#pragma once

class MathInstance : public CommonClassInstance
{ // Экземпляр "математического класса" - специального встроенного объекта с предопределенным
  // набором методов - коллекции математических функций.
public:
    using MathCallMethod = ObjectHolder(MathInstance::*)(const std::string&, const std::vector<ObjectHolder>&,
                                                         Context&);
    MathInstance() = default;
    void Print(std::ostream& os, Context& context) override;
    /*
     * Вызывает определённый математический метод method, передавая ему actual_args параметров.
     * Параметр context задаёт контекст для выполнения метода. Если метод method не существует,
     * метод выбрасывает исключение runtime_error.
     * Набор методов, входящих в коллекцию на данный момент, следующий:
     * abs(arg) - модуль числа arg.
     * pow(arg, exp) - возведение числа arg в степень exp.
     * sqrt(arg) - извлечение квадратного корня из arg.
     * sin(arg) - синус arg.
     * cos(arg) - косинус arg.
     * atan(arg) - арктангенс arg.
     * atan2(y, x) - арктангенс y / x (аналог функции atan2() из STL C++).
     * log(arg) - натуральный логарифм arg.
     * exp(arg) - нтуральный антилогарифм - возведение в степень arg основания натуральных логарифмов e.
     * ceil(arg) - округление аргумента arg вверх
     * floor(arg) - округление аргумента arg вниз
     * round(arg) - округление аргумента arg к ближайшему целому
     */
    ObjectHolder Call(const std::string& method_name, const std::vector<ObjectHolder>& actual_args,
                      Context& context, const std::string& parent_name = {}) override;
    bool HasMethod(const std::string& method_name, size_t argument_count) const override;

    [[nodiscard]] std::string GetClassName(void) const override
    {
        return "math";
    }

private:
    static const std::unordered_map<std::string_view, MathCallMethod> math_method_table_;
    static const std::unordered_map<std::string_view, std::pair<size_t, size_t>> math_method_argument_count_;

    // Обработчики методов класса "математическая коллекция"
    ObjectHolder MethodAbs(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                           Context& context);
    ObjectHolder MethodPow(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                           Context& context);
    ObjectHolder MethodSqrt(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                            Context& context);
    ObjectHolder MethodSin(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                           Context& context);
    ObjectHolder MethodCos(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                           Context& context);
    ObjectHolder MethodAtan(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                            Context& context);
    ObjectHolder MethodAtan2(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                             Context& context);
    ObjectHolder MethodLog(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                           Context& context);
    ObjectHolder MethodExp(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                           Context& context);
    ObjectHolder MethodCeil(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                            Context& context);
    ObjectHolder MethodFloor(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                             Context& context);
    ObjectHolder MethodRound(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                             Context& context);
};

class StringOpsInstance : public CommonClassInstance
{ // Экземпляр класса строковых операций - встроенного объекта для выполнения различных преобразований строковых переменных
  // и значений элементарного типа данных runtime::String языка МУФЛОН.
public:
    using StringOpsCallMethod = ObjectHolder(StringOpsInstance::*)(const std::string&, const std::vector<ObjectHolder>&,
                                                                   Context&);
    StringOpsInstance() = default;
    void Print(std::ostream& os, Context& context) override;
    /*
     * Вызывает определённый обрабатывающий строку метод method, передавая ему actual_args параметров.
     * Параметр context задаёт контекст для выполнения метода. Если метод method не существует,
     * метод выбрасывает исключение runtime_error.
     * Набор методов, входящих в коллекцию на данный момент, следующий:
     * size(arg_str) - длина строки arg_str.
     * length(arg_str) - длина строки arg_str - аналог (псевдоним) функции size(arg_str).
     * concat(arg_str_1, arg_str_2) - конкатенация (сцепление) строк arg_str_1 и arg_str_2.
     * append(arg_str_to, arg_str_what, arg_pos, arg_count) - присоединение к строке arg_str_to arg_count символов строки arg_str_what, начиная с позиции arg_pos в ней.
     * substr(arg_str, arg_pos, arg_length) - извлечение подстроки из строки arg_str длиной не более arg_length символов, начиная с символа с индексом arg_pos.
     // Функции поиска подстрок.
     * find(arg_str_haystack, arg_str_needle, arg_pos) - поиск первого вхождения подстроки arg_str_needle в строку arg_str_haystack, начиная с позиции arg_pos.
     * rfind(arg_str_haystack, arg_str_needle, arg_pos) - поиск последнего вхождения подстроки arg_str_needle в строку arg_str_haystack, начиная с позиции arg_pos.
     * find_first_of(arg_str_haystack, arg_str_needle_list, arg_pos) - поиск первого вхождения любого символа строки arg_str_needle в строку arg_str_haystack,
                                                                       начиная с позиции arg_pos.
     * find_first_not_of(arg_str_haystack, arg_str_needle_list, arg_pos) - поиск первого вхождения любого символа, не принадлежащего строке arg_str_needle,
                                                                           в строку arg_str_haystack, начиная с позиции arg_pos.
     * find_last_of(arg_str_haystack, arg_str_needle_list, arg_pos) - поиск последнего вхождения любого символа строки arg_str_needle в строку arg_str_haystack,
                                                                      заканчивая поиск позицей arg_pos.
     * find_last_not_of(arg_str_haystack, arg_str_needle_list, arg_pos) - поиск последнего вхождения любого символа, не принадлежащего строке arg_str_needle,
                                                                          в строку arg_str_haystack, заканчивая поиск позицей arg_pos.
     // Поисковые предикаты, проверяющие определенный тип вхождения одной строки в другую.
     * starts_with(arg_str_test, arg_str_start) - проверка, начинается ли строка arg_str_test с arg_str_start.
     * ends_with(arg_str_test, arg_str_end) - проверка, заканчивается ли строка arg_str_test строкой arg_str_end.
     * contains(arg_str_haystack, arg_str_needle) - проверка вхождения строки arg_str_needle в строку arg_str_haystack.
     // Функции частичной модификации строки arg_str путём замены её подстрок.
     * insert(arg_str, arg_pos, arg_str_ins, arg_count) - вставка строки arg_str_ins в количестве arg_count экземпляров в строку arg_str в положение arg_pos.
     * erase(arg_str, arg_pos, arg_length) - удаление arg_length символов из строки arg_str, начиная с положения arg_pos в ней.
     * replace(arg_str, arg_pos, arg_count, arg_str_ins, arg_pos_ins, arg_count_ins) - замена arg_count символов строки arg_str, начиная с положения arg_pos, на
                                                                                       arg_count_ins символов строки arg_str_ins, взятых с позиции arg_pos_ins в ней.
     * replicate(arg_str, arg_count) - конструирование строки из arg_count копий строки arg_str.
     * reverse(arg_str) - обращение (реверсирование) строки-аргумента arg_str.
     // Анализ и генерация кодов отдельных символов строки.
     * asc(arg_str, arg_pos) - получение ASCII-кода символа строки arg_str, находящегося в позиции arg_pos.
     * chr(arg_code, arg_code, ...) - генерация строки из символов с ASCII-кодами arg_code.
     // Преобразование из/в численного представления числа в/из строковое.
     * to_number(arg_str, arg_pos, base_value) - преобразование в числовую форму фрагмента строки arg_str, начинающегося с arg_pos, представляющего некоторое
     *                                           число в base_value-ичной системе счисления.
     * to_number_length() - возврат длины подстроки, которую удалось преобразовать в число в ходе последнего вызова to_number().
     * to_number_error() - код ошибки, которая могла возникнуть при последнем вызове to_number().
     * to_string(arg_number, base_value, double_precision) - преобразование в строку числового аргумента arg_number. Возможное назначение системы счисления (base_value
                                                             для целых чисел), а также формы представления и точности для чисел дробных (base_value и double_precision).
     * not_found() - метод без аргументов, всегда возвращает константу, которой поисковые методы (...find...) сигнализируют о неудачном поиске (найти
     *               искомый образец не удалось).
     */
    ObjectHolder Call(const std::string& method_name, const std::vector<ObjectHolder>& actual_args,
                      Context& context, const std::string& parent_name = {}) override;
    bool HasMethod(const std::string& method_name, size_t argument_count) const override;

    [[nodiscard]] std::string GetClassName(void) const override
    {
        return "string_ops";
    }

private:
    static const std::unordered_map<std::string_view, StringOpsCallMethod> string_ops_method_table_;
    static const std::unordered_map<std::string_view, std::pair<size_t, size_t>> string_ops_method_argument_count_;

    int last_to_number_error_ = 0;      // Ошибка, возникшая при последнем to_number().
    int last_to_number_length_ = 0;     // Длина фрагмента, использованного при последнем to_number().

    // Функция извлечения пары параметров подстроки - начального её индекса и длины.
    std::pair<size_t, size_t> ExtractPosSize
        (const std::vector<ObjectHolder>& actual_args, size_t arg_start_pos, const std::string& arg_str, Context& context);

    // Функция обобщённого поиска подстроки в строке, который для каждого конкретной разновидности отличается только передаваемой
    // поисковой функцией find_func.
    using CommonFindFunc = std::string::size_type(std::string::*)(const std::string& str, const std::string::size_type pos) const;
    ObjectHolder MethodCommonFind
        (const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context, size_t default_pos,
         CommonFindFunc find_func);

    // Функции-члены, представляющие собой реализацию внешних методов класса StringOpsInstance для МУФЛОН-программы.
    ObjectHolder MethodSize(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodConcat(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodAppend(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodSubstr(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    //
    ObjectHolder MethodFind(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodRFind(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodFindFirstOf(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodFindFirstNotOf(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodFindLastOf(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodFindLastNotOf(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodStartsWith(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodEndsWith(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodContains(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodNotFound(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    //
    ObjectHolder MethodInsert(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodErase(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodReplace(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodReplicate(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodReverse(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    //
    ObjectHolder MethodAsc(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodChr(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    //
    ObjectHolder MethodToNumber(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodToNumberLength(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodToNumberError(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
    ObjectHolder MethodToString(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);
};

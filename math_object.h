
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
    ObjectHolder Call(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                      Context& context) override;
    bool HasMethod(const std::string& method_name, size_t argument_count) const override;

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

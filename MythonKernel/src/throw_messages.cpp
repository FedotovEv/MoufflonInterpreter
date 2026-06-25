#include "throw_messages.h"

using namespace std;

namespace runtime
{
    const unordered_map<ThrowMessageNumber, string> ThrowMessages::throw_messages_
    {
        // Общие системные ошибки.
        {ThrowMessageNumber::THRM_MEMORY_ALLOCATION_ERROR, "Сбой при выделении динамической памяти"s},
        // Общие синтаксические ошибки конструкций исходного текста программы.
        {ThrowMessageNumber::THRM_SYNTAX_ERROR, "Общая синтаксическая ошибка"s},
        {ThrowMessageNumber::THRM_UNKNOWN, "Неопределённое исключение"s},
        // Особые случаи несогласованности аргументов по количеству или типу для некоторых специфических методов или классов.
        {ThrowMessageNumber::THRM_ARRAY_MUST_HAVE_DIMS, "Массив должен иметь одну или более размерностей"s},
        {ThrowMessageNumber::THRM_MAP_CTOR_HAS_NO_PARAMS, "Конструктор map не должен иметь аргументов"s},
        {ThrowMessageNumber::THRM_STR_HAS_ONE_PARAM, "Функция str должна иметь ровно один аргумент"s},
        {ThrowMessageNumber::THRM_IS_SAME_TARGET_HAS_TWO_PARAMS, "Функция is_same_target должна иметь ровно два параметра"s},
        {ThrowMessageNumber::THRM_MATH_CTOR_HAS_NO_PARAMS, "Конструктор math не должен иметь аргументов"s},
        {ThrowMessageNumber::THRM_STRINGOPS_CTOR_HAS_NO_PARAMS, "Конструктор класса строковых преобразователей string_ops не имеет аргументов"s},
        {ThrowMessageNumber::THRM_OBJECT_CTOR_HAS_NO_PARAMS, "Конструктор объекта-втыкалы не имеет аргументов"s},
        // Обращение к отсутствующим методам, переменным, полям или ссылкам.
        {ThrowMessageNumber::THRM_METHOD_NOT_FOUND, "Метод не найден"s},
        {ThrowMessageNumber::THRM_FREE_FUNCTION_NOT_FOUND, "Свободная функция не найдена"s},
        {ThrowMessageNumber::THRM_FIELD_NOT_FOUND, "Обращение к несуществующему или недоступному полю"s},
        {ThrowMessageNumber::THRM_INDIRECT_ASSIGN_ERROR, "Ошибка семантики косвенного присваивания"s},
        {ThrowMessageNumber::THRM_VARIABLE_NOT_FOUND, "Переменная не найдена"s},
        {ThrowMessageNumber::THRM_POINTER_RET_TO_VAL_DENIED, "Возврат указателя на значение запрещён"s},
        {ThrowMessageNumber::THRM_POINTER_RET_TOL_LOCAL_VAR_DENIED, "Возврат указателя на локальную переменную запрещён"s},
        // Невыполнимые или неопределённые операции.
        {ThrowMessageNumber::THRM_IMPOSSIBLE_ADDITION, "Невозможно выполнить сложение"s},
        {ThrowMessageNumber::THRM_IMPOSSIBLE_SUBTRACTION, "Невозможно выполнить вычитание"s},
        {ThrowMessageNumber::THRM_IMPOSSIBLE_MULTIPLICATION, "Невозможно выполнить умножение"s},
        {ThrowMessageNumber::THRM_IMPOSSIBLE_DIVISION, "Невозможно выполнить деление"s},
        {ThrowMessageNumber::THRM_IMPOSSIBLE_COMPARE_EQUAL, "Невозможно сравнить объекты на равенство"s},
        {ThrowMessageNumber::THRM_IMPOSSIBLE_COMPARE_LESS, "Невозможно сравнить объекты на \"меньше\""s},
        {ThrowMessageNumber::THRM_DIVISION_BY_ZERO, "Деление на нуль"s},
        {ThrowMessageNumber::THRM_IMPOSSIBLE_MODULO_DIVISION, "Невозможно выполнить взятие остатка от деления"s},
        {ThrowMessageNumber::THRM_MODULO_DIVISION_BY_ZERO, "Модульное деление на нуль"s},
        {ThrowMessageNumber::THRM_OVERFLOW, "Математическое переполнение"s},
        {ThrowMessageNumber::THRM_SHIFT_INVALID_PARAMS, "Недопустимые аргументы операции сдвига"s},
        {ThrowMessageNumber::THRM_NUMBER_STRING_CONVERSION_ERROR, "Ошибка при преобразовании числа в строку или обратно"s},
        {ThrowMessageNumber::THRM_CONTEXT_OUT_FAIL, "Печать данных в контекст завершилась неудачно"s},
        // Ошибка при работе с массивами или словарями.
        {ThrowMessageNumber::THRM_ARRAY_SIZE_NOT_NUMERIC, "Количество элементов в массиве должно задаваться числами"s},
        {ThrowMessageNumber::THRM_INVALID_ARRAY_INDEX, "Недопустимое значение индекса массива"s},
        {ThrowMessageNumber::THRM_PUSH_BACK_ONE_DIM_ONLY, "Метод PushBack допустим только для одномерных массивов"s},
        {ThrowMessageNumber::THRM_BACK_ONE_DIM_ONLY, "Метод Back допустим только для одномерных массивов"s},
        {ThrowMessageNumber::THRM_POP_BACK_ONE_DIM_ONLY, "Метод PopBack допустим только для одномерных массивов"s},
        {ThrowMessageNumber::THRM_ARRAY_IS_EMPTY, "Массив пуст"s},
        {ThrowMessageNumber::THRM_CURSOR_IN_PROGRESS_INSERT, "Идет работа с курсорами. Вызов Insert невозможен"},
        {ThrowMessageNumber::THRM_CURSOR_IN_PROGRESS_ERASE, "Идет работа с курсорами. Вызов Erase невозможен"},
        // Общая несогласованность формальных и фактических параметров.
        {ThrowMessageNumber::THRM_PARAMS_TYPE_INCONSISTENCY, "Несогласованность типа параметров метода"s},
        {ThrowMessageNumber::THRM_INVALID_PARAMS_COUNT, "Неверное количество параметров метода"s},
        {ThrowMessageNumber::THRM_INVALID_PARAM_VALUE, "Недопустимое значение параметра"s},
        {ThrowMessageNumber::THRM_INVALID_PARAM_TYPE, "Недопустимый тип параметра"s},
        {ThrowMessageNumber::THRM_INVALID_PARAM_LENGTH, "Недопустимая длина параметра(строки)"s},
        // Ошибки при разборе директив и спецификаторов вызова методов.
        {ThrowMessageNumber::THRM_INCORRECT_TOKEN_LIST, "Ошибка в параметрах команды"s},
        {ThrowMessageNumber::THRM_INCLUDE_INVALID_PARAMS, "Ошибка в параметрах директивы include"s},
        {ThrowMessageNumber::THRM_QUALIFIER_NOT_ANCESTOR, "Объект-уточнитель не является классом-предком объекта"s},
        {ThrowMessageNumber::THRM_AMBIGUOUS_OVERLOAD, "Неоднозначность перегрузки методов"s},
        // Проблемы при загрузке динамической библиотеки с коллекцией втыкал, а также при её регистрации.
        {ThrowMessageNumber::THRM_INVALID_IMPORT_FILENAME, "Недопустимое имя импортируемой библиотеки"s},
        {ThrowMessageNumber::THRM_INVALID_PLUGIN_NAME, "Получено некорректное имя втыкалы"s},
        {ThrowMessageNumber::THRM_INVALID_PLUGIN_METHOD_LIST, "Недопустимый формат или содержание списка методов втыкалы"s},
        {ThrowMessageNumber::THRM_INCORRECT_METHOD_DEFINER, "Получен невалидный описатель парметров конкретного метода"s},
        {ThrowMessageNumber::THRM_DYNAMIC_LIBRARY_NOT_LOADED, "Ошибка при загрузке разделяемой библиотеки"s},
        {ThrowMessageNumber::THRM_LOAD_PLUGINS_LIST_NOT_FOUND, "Не найдена корневая функция-информатор библиотеки коллекции втыкал"s},
        {ThrowMessageNumber::THRM_INVALID_PLUGIN_INFO_FUNC, "Некорректное имя информирующей функции втыкалы"s},
        {ThrowMessageNumber::THRM_INVALID_PLUGIN_CALL_FUNC, "Некорректное имя вызывающей функции втыкалы"s},
        // Специфические ошибки при использовании сопрограмм.
        {ThrowMessageNumber::THRM_METHOD_NOT_COROUTINE, "Метод не является сопрограммой"s},
        {ThrowMessageNumber::THRM_SPECIAL_METHOD_CANT_COROUTINE, "Специальный метод не может быть сопрограммой"s},
        {ThrowMessageNumber::THRM_OBJECT_NOT_AWAITABLE, "Объект не относится к стопусловным(не является ждуном)"s},
        // Нормальные операции, при которых ошибки используются как регулярное средство передачи информации.
        {ThrowMessageNumber::THRM_RAISE_CALL, "Принудительный вызов исключения оператором raise"s},
        {ThrowMessageNumber::THRM_URGENT_TERMINATE, "Немедленное завершение программы"s},
        // Подстановочные фрагменты, предназначенные для формирования составных, более сложных, сообщений.
        {ThrowMessageNumber::THRM_BASE_CLASS, "Базовый класс "s},
        {ThrowMessageNumber::THRM_NOT_FOUND_FOR_CLASS, " не найден для класса "s},
        {ThrowMessageNumber::THRM_CLASS, "Класс "s},
        {ThrowMessageNumber::THRM_FUNCTION, "Функция "s},
        {ThrowMessageNumber::THRM_ALREADY_EXISTS, " уже существует"s},
        {ThrowMessageNumber::THRM_USE_MULTIPLE_TIMES, " используется многократно"s},
        {ThrowMessageNumber::THRM_METHOD, "Метод "s},
        {ThrowMessageNumber::THRM_ARGUMENTS, " аргументов"s},
        {ThrowMessageNumber::THRM_DEMAND_EQUAL, " требует "s},
        {ThrowMessageNumber::THRM_DEMAND_LESS_OR_EQUAL, " требует не более "s},
        {ThrowMessageNumber::THRM_DEMAND_GREATER_OR_EQUAL, " требует не менее "s},
        {ThrowMessageNumber::THRM_PARAMETER, "Параметр "s},
        {ThrowMessageNumber::THRM_OF_METHOD, " метода "s},
        {ThrowMessageNumber::THRM_HAVE_INCOMPATIBLE_TYPE, " имеет несоответствующий тип"s},
        {ThrowMessageNumber::THRM_DEMAND_ONE_ARGUMENT, " требует 1 аргумент"s},
        {ThrowMessageNumber::THRM_FIRST_PARAM_OF_METHOD, "Параметр 1 метода "s},
        {ThrowMessageNumber::THRM_MUST_BE_CURSOR, " должен быть курсором"s},
        {ThrowMessageNumber::THRM_IN_METHOD, "В методе "s},
        {ThrowMessageNumber::THRM_CURSOR_INVALID, " курсор недействителен"s},
    };

    const std::string& ThrowMessages::GetThrowText(ThrowMessageNumber throw_message_number)
    {
        if (throw_messages_.count(throw_message_number))
            return throw_messages_.at(throw_message_number);
        else
            return throw_messages_.at(ThrowMessageNumber::THRM_UNKNOWN);
    }

    // Функция генерации сообщения об ошибке по трафарету text_pattern. Подстановочные коды передаются в массиве throw_messages.
    std::string ThrowMessages::ConstructThrowText(const std::string& text_pattern, std::vector<ThrowMessageNumber> throw_messages)
    {
        std::string result;
        for (size_t i = 0; i < text_pattern.size(); ++i)
        {
            char c = text_pattern[i];
            if (c == '\\')
            { // Следующий символ экранируется - рассматривается как обычный символ строки, без служебного значения.
              // Сам же обратный слэш отбрасывается, если не является последним символом трафарета.
                if (++i < text_pattern.size())
                    result += text_pattern[i];
                else
                    result += c;
            }
            else if (c == '%')
            { // Местоблюститель для замены на элемент из throw_messages.
                if (++i < text_pattern.size())
                {
                    std::string pattern_tail = text_pattern.substr(i);
                    const char* start_index_symbol = pattern_tail.c_str();
                    char* end_index_symbol;
                    size_t msg_index = strtoul(start_index_symbol, &end_index_symbol, 10);                    
                    if (size_t end_index_strlen = end_index_symbol - start_index_symbol)
                    {
                        if (msg_index >= 0 && msg_index < throw_messages.size())
                            result += GetThrowText(throw_messages[msg_index]);
                        i += end_index_strlen;
                    }
                    else
                    { // После опознавателя '%' вообще нет корректного номера подстановки.
                      // В таком случае воспринимаем его как обыковенный знак.
                        --i;
                        result += c;
                    }
                }
                else
                { // Символ '%' - последний в строке. Присоединяем его к результату как есть, а затем выходим.
                    result += c;
                }
            }
            else
            { // Обыкновенный очередной символ выходной строки.
                result += c;
            }
        }

        return result;
    }
} // namespace runtime

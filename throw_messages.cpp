﻿#include "throw_messages.h"

using namespace std;

namespace runtime
{
    const unordered_map<ThrowMessageNumber, string> ThrowMessages::throw_messages_
    {
        {ThrowMessageNumber::THRM_UNKNOWN, "Неопределённое исключение"s},
        {ThrowMessageNumber::THRM_NOT_SUPPORT_FREE_FUNCTION, "Муфлон не поддерживает свободные функции, только методы классов: "s},
        {ThrowMessageNumber::THRM_ARRAY_MUST_HAVE_DIMS, "Массив должен иметь одну или более размерностей"s},
        {ThrowMessageNumber::THRM_MAP_CTOR_HAS_NO_PARAMS, "Конструктор map не должен иметь аргументов"s},
        {ThrowMessageNumber::THRM_STR_HAS_ONE_PARAM, "Функция str должна иметь ровно один аргумент"s},
        {ThrowMessageNumber::THRM_UNKNOWN_METHOD_CALL, "Вызов неизвестного метода "s},
        {ThrowMessageNumber::THRM_POINTER_RET_TO_VAL_DENIED, "Возврат указателя на значение запрещён"s},
        {ThrowMessageNumber::THRM_POINTER_RET_TOL_LOCAL_VAR_DENIED, "Возврат указателя на локальную переменную запрещён"s},
        {ThrowMessageNumber::THRM_BASE_CLASS, "Базовый класс "s},
        {ThrowMessageNumber::THRM_NOT_FOUND_FOR_CLASS, " не найден для класса "s},
        {ThrowMessageNumber::THRM_CLASS, "Класс "s},
        {ThrowMessageNumber::THRM_ALREADY_EXISTS, "уже сущестует"s},
        {ThrowMessageNumber::THRM_METHOD_NOT_FOUND, "Метод не найден"s},
        {ThrowMessageNumber::THRM_INDIRECT_ASSIGN_ERROR, "Ошибка семантики косвенного присваивания"s},
        {ThrowMessageNumber::THRM_VARIABLE_NOT_FOUND, "Переменная не найдена"s},
        {ThrowMessageNumber::THRM_IMPOSSIBLE_ADDITION, "Невозможно выполнить сложение"s},
        {ThrowMessageNumber::THRM_IMPOSSIBLE_SUBTRACTION, "Невозможно выполнить вычитание"s},
        {ThrowMessageNumber::THRM_IMPOSSIBLE_MULTIPLICATION, "Невозможно выполнить умножение"s},
        {ThrowMessageNumber::THRM_IMPOSSIBLE_DIVISION, "Невозможно выполнить деление"s},
        {ThrowMessageNumber::THRM_IMPOSSIBLE_COMPARE_EQUAL, "Невозможно сравнить объекты на равенство"s},
        {ThrowMessageNumber::THRM_IMPOSSIBLE_COMPARE_LESS, "Невозможно сравнить объекты на \"меньше\""s},
        {ThrowMessageNumber::THRM_DIVISION_BY_ZERO, "Деление на нуль"s},
        {ThrowMessageNumber::THRM_IMPOSSIBLE_MODULO_DIVISION, "Невозможно выполнить взятие остатка от деления"s},
        {ThrowMessageNumber::THRM_MODULO_DIVISION_BY_ZERO, "Модульное деление на нуль"s},
        {ThrowMessageNumber::THRM_NOT_DIGIT_SIZES, "Количество элементов в массиве должно задаваться числами"s},
        {ThrowMessageNumber::THRM_INVALID_ARRAY_INDEX, "Недопустимое значение индекса массива"s},
        {ThrowMessageNumber::THRM_PUSH_BACK_ONE_DIM_ONLY, "Метод PushBack допустим только для одномерных массивов"s},
        {ThrowMessageNumber::THRM_BACK_ONE_DIM_ONLY, "Метод Back допустим только для одномерных массивов"s},
        {ThrowMessageNumber::THRM_POP_BACK_ONE_DIM_ONLY, "Метод PopBack допустим только для одномерных массивов"s},
        {ThrowMessageNumber::THRM_ARRAY_IS_EMPTY, "Массив пуст"s},
        {ThrowMessageNumber::THRM_ITERATOR_IN_PROGRESS_INSERT, "Идет работа с итераторами. Вызов Insert невозможен"},
        {ThrowMessageNumber::THRM_ITERATOR_IN_PROGRESS_ERASE, "Идет работа с итераторами. Вызов Erase невозможен"},
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
        {ThrowMessageNumber::THRM_MUST_BE_ITERATOR, " должен быть итератором"s},
        {ThrowMessageNumber::THRM_IN_METHOD, "В методе "s},
        {ThrowMessageNumber::THRM_ITERATOR_INVALID, " итератор недействителен"s},
        {ThrowMessageNumber::THRM_MATH_CTOR_HAS_NO_PARAMS, "Конструктор math не должен иметь аргументов"s},
        {ThrowMessageNumber::THRM_INCORRECT_TOKEN_LIST, "Ошибка в параметрах команды"s}
    };
} // namespace runtime

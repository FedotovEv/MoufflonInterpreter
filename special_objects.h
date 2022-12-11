#pragma once

//Итератор для словаря (ассоциативного массива)
class MapInstance;
class MapIterator : public Object
{
public:
    MapIterator(MapInstance& map_instance, std::map<std::string, ObjectHolder>& map_storage);
    void Print(std::ostream& os, Context& context) override;
    const void* GetPtr() const
    {
        return nullptr;
    }

    size_t SizeOf() const
    {
        return 0;
    }
    bool IsIteratorValid();
    ObjectHolder IteratorGetKey();
    ObjectHolder IteratorGetValue();
    bool Begin();
    bool IteratorLowerBound(const std::string& map_key);
    bool IteratorNext();
    bool IteratorPrevious();
    bool IsIteratorEnd();
    bool IsIteratorBegin();

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
                      Context& context) override;

private:
    static const std::unordered_map<std::string_view, ArrayCallMethod> array_method_table_;

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
     * next(iterator) - возвращает итератор, указывающий на элемент словаря, следующий после iterator.
     * key(iterator) - возвращает ключ элемента, соответствующего iterator.
     * value(iterator) - чтение или изменение элемента, на который указывает iterator.
     * release() - сообщает об окончании процесса перечисления элементов словаря
     */
    ObjectHolder Call(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                      Context& context) override;
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
    ObjectHolder MethodIsIteratorBegin(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                       Context& context);
    ObjectHolder MethodIsIteratorEnd(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                     Context& context);
    ObjectHolder MethodRelease(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                               Context& context);

    std::map<std::string, ObjectHolder> map_storage_;
    bool is_in_iterator_mode_ = false;
    int iterator_pack_serial_;

    static int last_iterator_pack_serial_;
};


#pragma once

//�������� ��� ������� (�������������� �������)
class MapInstance;
class MapIterator : public Object
{
public:
    MapIterator(MapInstance& map_instance, std::map<std::string, ObjectHolder>& map_storage);
    void Print(std::ostream& os, Context& context) override;
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
{ // ��������� ������� - ������������ ����������� ������� � ���������������� ������� �������.
public:

    using ArrayCallMethod = ObjectHolder(ArrayInstance::*)(const std::string&, const std::vector<ObjectHolder>&,
        Context&);
    ArrayInstance(std::vector<int> elements_count);
    void Print(std::ostream& os, Context& context) override;
    /*
     * �������� � �������-������� ����� method, ��������� ��� actual_args ����������.
     * �������� context ����� �������� ��� ���������� ������. ���� ����� method �� ��������� � ���,
     * ������� ������������ ������, ����� ����������� ���������� runtime_error.
     * ����� �������, �������������� ��������, ���������:
     * get(... ������� ...) -
     *      - ������ ��� ���������� � ��������� �������� �������� �������, �������������
     *      ������� ���������-��������. ��� ������� ���������� � ���� (�����������
     *      ������ �������� ��� ������ ����������� ����� 0).
     * get_array_dimensions() - ��������� ���������� ��������� �������.
     * get_dimension_count(dimension_number) -
     *      - ��������� ���������� ��������� ��� ����������� dimension_number.
     *      ����� ����������� ���������� � 1 (������� ������ �
     *      ��������������� ����������� ����� ����� 1).
     * resize(... ���������� ��������� �� ������������ ...) -
     *      - ������������ ������� � ���� ������������ � ����������� ���������.
     * ��������� ������ ���������� ������ ��� ���������� ��������. ��� ����������� �������� ����� ���������
     * ���������� runtime_error.
     * push_back(new_element) - ��������� ������� new_element � ����� �������.
     * back() - ��������� ������� ��� ���������� ��������� ������� �������.
     * pop_back() - ������� �� ������� ��������� �������
     */
    ObjectHolder Call(const std::string& method, const std::vector<ObjectHolder>& actual_args,
        Context& context) override;

private:
    static const std::unordered_map<std::string_view, ArrayCallMethod> array_method_table_;

    // ����������� ������� ������ "������"
    ObjectHolder MethodGet(const std::string& method, const std::vector<ObjectHolder>& actual_args,
        Context& context);
    ObjectHolder MethodGetArrayDimensions(const std::string& method, const std::vector<ObjectHolder>& actual_args,
        Context& context);
    ObjectHolder MethodGetDimensionCount(const std::string& method, const std::vector<ObjectHolder>& actual_args,
        Context& context);
    ObjectHolder MethodResize(const std::string& method, const std::vector<ObjectHolder>& actual_args,
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
{ // ��������� �������������� ������� (�������) - ������������ ����������� ������� � ���������������� ������� �������.
public:

    using MapCallMethod = ObjectHolder(MapInstance::*)(const std::string&, const std::vector<ObjectHolder>&,
        Context&);
    MapInstance() = default;
    void Print(std::ostream& os, Context& context) override;
    /*
     * �������� � �������-������� ����� method, ��������� ��� actual_args ����������.
     * �������� context ����� �������� ��� ���������� ������. ���� ����� method �� ��������� � ���,
     * ������� ������������ �������, ����� ����������� ���������� runtime_error.
     * ����� �������, �������������� ��������, ���������:
     * insert(key) - ������� � ������ �������� � ������ key.
     * find(key) - ������ ��� ��������� ��� ������������� �������� � ������ key.
     * erase(key) - �������� �������� � ������ key.
     * contains(key) - �������� ������� �������� � ������ key.
     * begin() - ������� "���������", ������������ �� ������ ������� �������.
     * next(iterator) - ���������� ��������, ����������� �� ������� �������, ��������� ����� iterator.
     * key(iterator) - ���������� ���� ��������, ���������������� iterator.
     * value(iterator) - ������ ��� ��������� ��������, �� ������� ��������� iterator.
     * release() - �������� �� ��������� �������� ������������ ��������� �������
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

    // ����������� ������� ������ "������������� ������(�������)"
    ObjectHolder MethodInsert(const std::string& method, const std::vector<ObjectHolder>& actual_args,
        Context& context);
    ObjectHolder MethodFind(const std::string& method, const std::vector<ObjectHolder>& actual_args,
        Context& context);
    ObjectHolder MethodErase(const std::string& method, const std::vector<ObjectHolder>& actual_args,
        Context& context);
    ObjectHolder MethodContains(const std::string& method, const std::vector<ObjectHolder>& actual_args,
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


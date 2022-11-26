#include "throw_messages.h"

using namespace std;

namespace runtime
{
    const unordered_map<ThrowMessageNumber, string> ThrowMessages::throw_messages_
    {
        {ThrowMessageNumber::THRM_UNKNOWN, "������������� ����������"s},
        {ThrowMessageNumber::THRM_NOT_SUPPORT_FREE_FUNCTION, "������ �� ������������ ��������� �������, ������ ������ �������: "s},
        {ThrowMessageNumber::THRM_ARRAY_MUST_HAVE_DIMS, "������ ������ ����� ���� ��� ����� ������������"s},
        {ThrowMessageNumber::THRM_MAP_CTOR_HAS_NO_PARAMS, "����������� map �� ������ ����� ����������"s},
        {ThrowMessageNumber::THRM_STR_HAS_ONE_PARAM, "������� str ������ ����� ����� ���� ��������"s},
        {ThrowMessageNumber::THRM_UNKNOWN_METHOD_CALL, "����� ������������ ������ "s},
        {ThrowMessageNumber::THRM_POINTER_RET_TO_VAL_DENIED, "������� ��������� �� �������� ��������"s},
        {ThrowMessageNumber::THRM_POINTER_RET_TOL_LOCAL_VAR_DENIED, "������� ��������� �� ��������� ���������� ��������"s},
        {ThrowMessageNumber::THRM_BASE_CLASS, "������� ����� "s},
        {ThrowMessageNumber::THRM_NOT_FOUND_FOR_CLASS, " �� ������ ��� ������ "s},
        {ThrowMessageNumber::THRM_CLASS, "����� "s},
        {ThrowMessageNumber::THRM_ALREADY_EXISTS, "��� ���������"s},
        {ThrowMessageNumber::THRM_METHOD_NOT_FOUND, "����� �� ������"s},
        {ThrowMessageNumber::THRM_INDIRECT_ASSIGN_ERROR, "������ ��������� ���������� ������������"s},
        {ThrowMessageNumber::THRM_VARIABLE_NOT_FOUND, "���������� �� �������"s},
        {ThrowMessageNumber::THRM_IMPOSSIBLE_ADDITION, "���������� ��������� ��������"s},
        {ThrowMessageNumber::THRM_IMPOSSIBLE_SUBTRACTION, "���������� ��������� ���������"s},
        {ThrowMessageNumber::THRM_IMPOSSIBLE_MULTIPLICATION, "���������� ��������� ���������"s},
        {ThrowMessageNumber::THRM_IMPOSSIBLE_DIVISION, "���������� ��������� �������"s},
        {ThrowMessageNumber::THRM_IMPOSSIBLE_COMPARE_EQUAL, "���������� �������� ������� �� ���������"s},
        {ThrowMessageNumber::THRM_IMPOSSIBLE_COMPARE_LESS, "���������� �������� ������� �� \"������\""s},
        {ThrowMessageNumber::THRM_DIVISION_BY_ZERO, "������� �� ����"s},
        {ThrowMessageNumber::THRM_IMPOSSIBLE_MODULO_DIVISION, "���������� ��������� ������ ������� �� �������"s},
        {ThrowMessageNumber::THRM_MODULO_DIVISION_BY_ZERO, "��������� ������� �� ����"s},
        {ThrowMessageNumber::THRM_NOT_DIGIT_SIZES, "���������� ��������� � ������� ������ ���������� �������"s},
        {ThrowMessageNumber::THRM_INVALID_ARRAY_INDEX, "������������ �������� ������� �������"s},
        {ThrowMessageNumber::THRM_PUSH_BACK_ONE_DIM_ONLY, "����� PushBack �������� ������ ��� ���������� ��������"s},
        {ThrowMessageNumber::THRM_BACK_ONE_DIM_ONLY, "����� Back �������� ������ ��� ���������� ��������"s},
        {ThrowMessageNumber::THRM_POP_BACK_ONE_DIM_ONLY, "����� PopBack �������� ������ ��� ���������� ��������"s},
        {ThrowMessageNumber::THRM_ARRAY_IS_EMPTY, "������ ����"s},
        {ThrowMessageNumber::THRM_ITERATOR_IN_PROGRESS_INSERT, "���� ������ � �����������. ����� Insert ����������"},
        {ThrowMessageNumber::THRM_ITERATOR_IN_PROGRESS_ERASE, "���� ������ � �����������. ����� Erase ����������"},
        {ThrowMessageNumber::THRM_METHOD, "����� "s},
        {ThrowMessageNumber::THRM_ARGUMENTS, " ����������"s},
        {ThrowMessageNumber::THRM_DEMAND_EQUAL, " ������� "s},
        {ThrowMessageNumber::THRM_DEMAND_LESS_OR_EQUAL, " ������� �� ����� "s},
        {ThrowMessageNumber::THRM_DEMAND_GREATER_OR_EQUAL, " ������� �� ����� "s},
        {ThrowMessageNumber::THRM_PARAMETER, "�������� "s},
        {ThrowMessageNumber::THRM_OF_METHOD, " ������ "s},
        {ThrowMessageNumber::THRM_HAVE_INCOMPATIBLE_TYPE, " ����� ����������������� ���"s},
        {ThrowMessageNumber::THRM_DEMAND_ONE_ARGUMENT, " ������� 1 ��������"s},
        {ThrowMessageNumber::THRM_FIRST_PARAM_OF_METHOD, "�������� 1 ������ "s},
        {ThrowMessageNumber::THRM_MUST_BE_ITERATOR, " ������ ���� ����������"s},
        {ThrowMessageNumber::THRM_IN_METHOD, "� ������ "s},
        {ThrowMessageNumber::THRM_ITERATOR_INVALID, " �������� ��������������"s}
    };
} // namespace runtime

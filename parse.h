#pragma once

#include "declares.h"
#include <memory>
#include <stdexcept>

#include "throw_messages.h"

namespace parse
{
    class Lexer;
}

namespace runtime
{
    class Executable;
}

struct ParseError : std::runtime_error
{
    using std::runtime_error::runtime_error;
    ParseError(runtime::ThrowMessageNumber throw_message_number);
};

// �� ������ ������ �������������� ��� ������� �������� ������� �������� ���������� ("������") -
// - �� ����� ����������� ����������� ���������� (.DLL ��� .SO, � ����������� �� ������������ �������),
// � ����� ��������������� �� ������, ���� ���� ������ ��������-����������� �������� ��� �����������
// ������� ��������, ������� ���������� � ���������� ���� �������������.
// ���� LoadLibraryDefine �������� string - ������� ����������� �� ����������� ����������, ��� �������
// ��������� � ���� �������. ��� ��� ��� ���������� ��������� ����� �������� ������ ������������ ��������
// ������������ ������� (LoadLibraryW ��� ������� ��� dlopen ��� �������).
// ���� �� LoadLibraryDefine �������� ��� InternalObjectCreator - ������� ������������, ���������
// "��������� �����", ��������� �� ������� � ���� �������� LoadLibraryDefine.
// � ������, ����� LoadLibraryDefine �������� monostate, ������� �� �����������.

using FuncInternalObjectCreator = std::unique_ptr<runtime::Executable>
                        (std::vector<std::unique_ptr<runtime::Executable>>);
using InternalObjectCreator = std::function<FuncInternalObjectCreator>;
using LoadLibraryDefine = std::variant<std::monostate, std::string, InternalObjectCreator>;

class ParseContext
{
public:
    virtual LoadLibraryDefine GetLoadLibraryDesc(const std::string& library_name) const = 0;
};

std::unique_ptr<runtime::Executable> ParseProgram(parse::Lexer& lexer);
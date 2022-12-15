#pragma once

#include "declares.h"
#include <memory>
#include <stdexcept>

#include "throw_messages.h"

namespace ast
{
    class RootCompound;
}

namespace runtime
{
    class Executable;
}

// �� ������ ������ �������������� ��� ������� �������� ������� �������� ���������� ("������") -
// - �� ����� ����������� ����������� ���������� (.DLL ��� .SO, � ����������� �� ������������ �������),
// � ����� ��������������� �� ������, ���� ���� ������ ��������-����������� �������� ��� �����������
// ������� ��������, ������� ���������� � ���������� ���� �������������.
// ���� LoadLibraryDefine �������� string - ������� ����������� �� ����������� ����������, ��� �������
// ��������� � ���� �������. ��� ��� ��� ���������� ��������� ����� �������� ������ ������������ ��������
// ������������ ������� (LoadLibraryW ��� ������� ��� dlopen ��� �������).
// ���� �� LoadLibraryDefine �������� ��� InternalObjectCreatorList - ������� ������������, ���������
// ���� �� ����� ������ � ��������� �� "��������� �����", ������� � ���������� � ������� LoadLibraryDefine.
// � ������, ����� LoadLibraryDefine �������� monostate, ������� �� �����������.

using FuncInternalObjectCreator = std::unique_ptr<runtime::Executable>
                            (std::vector<std::unique_ptr<runtime::Executable>>);
using InternalObjectCreator = std::function<FuncInternalObjectCreator>;
using InternalObjectCreatorList = std::vector<std::pair<std::string, InternalObjectCreator>>;
using LoadLibraryDefine = std::variant<std::monostate, std::string, InternalObjectCreatorList>;

namespace parse
{
    class Lexer;
    class ParseContext
    {
    public:
        virtual ~ParseContext() = default;
        virtual LoadLibraryDefine GetLoadLibraryDesc(const std::string& library_name) const = 0;
    };

    struct GlobalResourceInfo
    {
        #if defined (_WIN64) || defined(_WIN32)
            std::vector<HMODULE> dll_list;
        #elif defined(__unix__) || defined(__linux__) || defined(__USE_POSIX)
            std::vector<void*> dll_list;
        #endif
    };
}

struct ParseError : std::runtime_error
{
    using std::runtime_error::runtime_error;
    ParseError(runtime::ThrowMessageNumber throw_message_number);
};

std::unique_ptr<runtime::Executable> ParseProgram(parse::Lexer& lexer);
parse::GlobalResourceInfo GetGlobalResourceList(const std::unique_ptr<runtime::Executable>& node);
void DeallocateGlobalResources(const parse::GlobalResourceInfo& global_resources);

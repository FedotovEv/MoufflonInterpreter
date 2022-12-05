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

enum class LoadLibraryType
{
    LOAD_LIBRARY_DONT_LOAD = 0,
    LOAD_LIBRARY_FROM_DLL_FILE,
    LOAD_LIBRARY_BY_ADDRESS    
};

struct LoadLibraryDescriptor
{
    using LoadLibraryCreateFunc = std::function<std::unique_ptr<runtime::Executable>
                                       (std::vector<std::unique_ptr<runtime::Executable>>)>;
    LoadLibraryType load_library_type;
    LoadLibraryCreateFunc create_func;
};

class ParseContext
{
public:
    virtual LoadLibraryDescriptor GetLoadLibraryDesc(const std::string& library_name) = 0;
};

std::unique_ptr<runtime::Executable> ParseProgram(parse::Lexer& lexer);